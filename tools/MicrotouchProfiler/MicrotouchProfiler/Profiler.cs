using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace MicrotouchProfiler
{
    class Profiler
    {
        public class Module
        {
            public string Name;
            public int Addr;
            public int Count;
        }

        public class SampleCount
        {
            public int Addr;
            public int Count;
        }

        int[] _samples = new int[0x8000];
	    int _count;
	    int _maxAddr;

	    int _module;
	    string _path;
        List<string> _lines = new List<string>();
        List<int> _lineToAddress = new List<int>();
        List<Module> _modules = new List<Module>();
	    Dictionary<int,int> _addrToLine = new Dictionary<int,int>();
	    Dictionary<int,int> _addrToModule = new Dictionary<int,int>();

        public List<SampleCount> PerAddr;
        public List<SampleCount> PerModule;
        public int ReportCount;
        public int MaxCount;

        public bool IsOpen()
        {
            return _path != null;
        }

        public void Clear()
        {
            _count = 0;
            for (int i = 0; i < 0x8000; i++)
                _samples[i] = 0;
        }

        bool ToInt(string addr, out int v)
        {
            v = 0;
            if (addr.Length == 0)
                return false;
            try
            {
                v = Convert.ToInt32(addr,16);
                return true;
            }
            catch (Exception)
            {
                Console.WriteLine(addr);
            }
            return false;
        }

	    int MapLine(string addr, int line)
	    {
		    int a;
            if (!ToInt(addr, out a))
                return -1;
		    _addrToLine[a] = line;
		    _addrToModule[a] = _module;
	        _maxAddr = a+2; // This is not strictly true; 4 bytes per line for some instructions
            return a;
	    }

	    void AddModule(string addr, string module)
	    {
		    if (module[0] != '<')
			    return;
		    int a;
		    if (!ToInt(addr,out a))
			    return;
		    _module = _modules.Count();
		    _modules.Add(new Module() { Name = module, Addr = a });
	    }

        public int AddressToLine(int addr)
        {
            if (!_addrToLine.ContainsKey(addr))
                return -1;
            return _addrToLine[addr];
        }

        public int LineToAddress(int line)
        {
            if (line < 0 || line >= _lineToAddress.Count)
                return -1;
            return _lineToAddress[line];
        }

        public int LineCount()
        {
            return _lines.Count;
        }

        public string GetLine(int line)
        {
            if (line < 0 || line >= _lines.Count)
                return null;
            return _lines[line];
        }

        //  Sort largest count first
        int scomp(SampleCount a, SampleCount b)
        {
            return b.Count - a.Count;
        }

        public bool Load(string path)
        {
            int[] samples = new int[0x8000];
            try
            {
                using (StreamReader r = new StreamReader(path))
                {
                    string s;
                    while ((s = r.ReadLine()) != null)
                    {
                        string[] ss = s.Split();
                        samples[Convert.ToInt32(ss[0],16) >> 1] = Convert.ToInt32(ss[1],16);
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                return false;
            }
            _samples = samples;
            return true;
        }

        public void Report()
        {
            for (int i = 0; i < _modules.Count(); i++)
			    _modules[i].Count = 0;

		//  Samples per address
            MaxCount = 0;
            List<SampleCount> samples = new List<SampleCount>();
		    for (int i = 0; i < _maxAddr; i+= 2)
		    {
			    int count = _samples[i>>1];
			    if (count == 0)
				    continue;
                if (count > MaxCount)
                    MaxCount = count;

			    _modules[_addrToModule[i]].Count += count;
                samples.Add(new SampleCount() { Addr = i, Count = count });
		    }
            samples.Sort(scomp);

		    // Samples per module
		    List<SampleCount> modules = new List<SampleCount>();
            ReportCount = 0;
		    for (int i = 0; i < _modules.Count(); i++)
		    {
			    int count = _modules[i].Count;
			    if (count == 0)
				    continue;
                ReportCount += count;
			    modules.Add(new SampleCount() { Addr = _modules[i].Addr, Count = _modules[i].Count });
		    }
            modules.Sort(scomp);

            //  A Report is a snapshot, samples are still inbound
            PerAddr = samples;
            PerModule = modules;
	    }

        public int GetSample(int addr)
        {
            return _samples[addr >> 1];
        }

        public string ModuleName(int addr)
        {
            return _modules[_addrToModule[addr]].Name;
        }

        public int ModuleEnd(int addr)
        {
            int m = _addrToModule[addr];
            if (m == _modules.Count - 1)
                return _maxAddr;
            return _modules[m + 1].Addr;
        }

	    public void Open(string path)
	    {
            using (StreamReader r = new StreamReader(path))
            {
		        _path = path;
                string s;
		        while ((s = r.ReadLine()) != null)
		        {
                    int line = _lines.Count();
			        _lines.Add(s);
                    int addr = -1;

                    int len = s.Length;
                    if (len != 0)
                    {
                        if (s[len - 1] == ':')
                        {
                            string[] ss = s.Split();
                            if (ss.Length >= 2 && ss[0].Length == 8)
                                AddModule(ss[0].Trim(), ss[1]);
                        }
                        else if (s.Length > 8 && s[8] == ':')
                        {
                            addr = MapLine(s.Substring(0, 8).Trim(), line);
                        }
                    }

                    _lineToAddress.Add(addr);
		        }
            }
	    }

        public void AddSample(int addr)
        {
            addr >>= 1;
            if (addr < 0 || addr >= 0x8000)
                return;
            _samples[addr]++;
            _count++;
        }
    }
}
