using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace MicrotouchProfiler
{
    class ObjectFile : List<Blob>
    {
        public int TotalSize;
        public string Name;
        public void AddBlob(Blob b)
        {
            TotalSize += b.Size;
            Add(b);
        }
        public override string ToString()
        {
            return TotalSize.ToString();
        }
    }

    class Blob
    {
        public string Name;
        public string Lib;
        public int Size;
        public override string ToString()
        {
            return Size.ToString() + " " + Name + " " + Lib;
        }
    }

    class MapFile : Dictionary<string, ObjectFile>
    {
        public string Name;
    }

    class MapGrinder
    {
        MapFile _map = new MapFile();
        public int TotalText;

        void Add(string name, string lib, string size)
        {
            Blob b = new Blob { Lib = lib, Name = name, Size = Convert.ToInt32(size, 16) };
            if (!_map.ContainsKey(b.Lib))
                _map[b.Lib] = new ObjectFile() { Name = b.Lib };
            _map[b.Lib].AddBlob(b);
        }

        void Add(string line)
        {
            List<string> clean = new List<string>();
            foreach (string s in line.Trim().Split(' '))
            {
                string st = s.Trim();
                if (st.Length > 0)
                {
                    if (clean.Count > 0 && st.StartsWith(".text"))
                        break;
                    clean.Add(st);
                }
            }
            if (clean[2].Trim() == "0x0")
                return;

            if (clean[0] == ".text" && clean[1] == "0x00000000")
            {
                TotalText = Convert.ToInt32(clean[2], 16);
                return;
            }

            if (clean[0].StartsWith(".rodata"))
            {
                Add(clean[0], clean[3], clean[2]);
                return;
            }

            if (clean[3].IndexOf("libc.a") != -1)
                clean[3] = "libc.a";
            if (clean[3].IndexOf("libgcc.a") != -1)
                clean[3] = "libgcc.a";
            if (clean[3].IndexOf("newlib") != -1)
                clean[3] = "newlib.a";
            while (clean.Count < 6)
                clean.Add(clean[0]);
            Add(clean[5], clean[3], clean[2]);

            //Console.WriteLine(b);
        }

        public MapFile Grind(string path)
        {
            List<string> lines = new List<string>();
            using (StreamReader sr = new StreamReader(path))
            {
                String line;
                while ((line = sr.ReadLine()) != null)
                    lines.Add(line);
            }

            int i;
            for (i = 0; i < lines.Count; i++)
            {
                string s = lines[i].Trim();
                if (s.StartsWith(".text") || s.StartsWith(".rodata") || s.StartsWith(".isr_vector") || s.StartsWith(".progmem.data"))
                    Add(s + " " + lines[i + 1] + " " + lines[i + 2]);
            }

            _map.Name = Path.GetFileNameWithoutExtension(path);
            return _map;
        }
    }
}
