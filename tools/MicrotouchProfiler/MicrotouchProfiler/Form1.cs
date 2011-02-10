using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.IO.Ports;
using System.Management;
using System.Windows.Forms;

namespace MicrotouchProfiler
{
    public partial class Form1 : Form
    {
        delegate void TextDelegate(string t);

        Profiler _profiler = new Profiler();
        Font _font = new Font("Lucida Console", 8);
        int _lineHeight;
        bool _serialRunning = false;
        int _sampleCount;

        public Form1()
        {
            InitializeComponent();
            vScrollBar1.ValueChanged += vScrollBar1_ValueChanged;
            panel1.Resize += panel1_Resize;
            _lineHeight = (int)_font.GetHeight() + 3;
            Report();
        }

        void CheckSerial()
        {
            if (_serialRunning)
                return;
            string com = MicrotouchSerial();
            if (com != null)
                OpenSerial(com);
            else
                SetStatus("Microtouch not found");
        }

        string MicrotouchSerial()
        {
            ManagementObjectSearcher searcher;
            searcher = new ManagementObjectSearcher("Select * from Win32_PnPEntity");
            foreach (ManagementObject obj in searcher.Get())
            {
               
                string description = (string)obj["Description"];
                if (description != null && description.StartsWith("Microtouch"))
                {
                    string name = (string)obj["Name"];
                    string[] names = name.Split();
                    string com = names[names.Length - 1];       // Last one
                    return com.Substring(1, com.Length - 2);    // extract com number
                }
            }
            return null;
        }

        void SetStatus(string s)
        {
            if (InvokeRequired)
            {
                this.Invoke(new TextDelegate(SetStatus), new object[] { s });
                return;
            }
            toolStripStatusLabel1.Text = s;
        }

        void OpenSerial(string name)
        {
            ThreadPool.QueueUserWorkItem(new WaitCallback(SerialReader),name);
        }

        void SerialReader(object state)
        {
            _serialRunning = true;
            string com = (string)state;
            try
            {
                using (SerialPort serial = new SerialPort(com))
                {
                    serial.Open();
                    byte[] b = Encoding.UTF8.GetBytes("p1" + ((char)0x0D));
                    serial.Write(b, 0, b.Length);
                    SetStatus("Microtouch connected on " + com);

                    for (; ; )
                    {
                        string s = serial.ReadLine().Trim();
                        if (s == null)
                            break;
                        if (s.Length == 4 && _profiler != null)
                        {
                            try
                            {
                                _profiler.AddSample(Convert.ToInt32(s, 16));
                                _sampleCount++;
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine(ex);
                            }
                        }
                        else
                        {
                            // Console.WriteLine(s);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                SetStatus("Failed to open Microtouch on " + com + ", try replugging");
            }
            _serialRunning = false;
        }

        private void openLssFileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog fd = new OpenFileDialog();
            fd.Filter = "Listing Files(*.lss)|*.lss|All files (*.*)|*.*";
            if (fd.ShowDialog() != DialogResult.OK)
                return;
            _profiler.Open(fd.FileName);
            UpdateCode();
            Report();
        }

        Color GetColor(int s, int count)
        {
            int c = 255;
            if (count != 0)
                c -= s * 1023 / count;
            if (c < 0)
                c = 0;
            return Color.FromArgb(0xFF, c, c);
        }

        string PercentString(int pc, int of)
        {
            int p = pc * 100 / of;
            if (p == 0)
            {
                if (pc != 0)
                    return "<1%";
                return "";
            }
            return p.ToString() + "%";
        }

        void UpdateCode()
        {
            if (_profiler == null)
                return;
            int page = (panel1.Height/_lineHeight)*_lineHeight;
            vScrollBar1.LargeChange = page;
            vScrollBar1.SmallChange = _lineHeight;
            vScrollBar1.Maximum = _lineHeight * _profiler.LineCount();
            panel1.Invalidate();
        }

        void panel1_Resize(object sender, EventArgs e)
        {
            UpdateCode();
        }

        void vScrollBar1_ValueChanged(object sender, EventArgs e)
        {
            panel1.Invalidate();
        }

        void Report()
        {
            CheckSerial();

            _profiler.Report();
            int count = _profiler.ReportCount;

            if (_serialRunning)
                SetStatus(String.Format("Microtouch connected ({0} samples)", _sampleCount));

            List<Profiler.SampleCount> perModule = _profiler.PerModule;
            listView1.SuspendLayout();
            listView1.Items.Clear();
            foreach (Profiler.SampleCount s in perModule)
            {
                string name = _profiler.ModuleName(s.Addr);
                ListViewItem item = listView1.Items.Add(PercentString(s.Count, count).PadRight(5) + name);
                item.Tag = s;
            }
            listView1.ResumeLayout();
            panel1.Invalidate();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            Report();
        }

        private void listView1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listView1.SelectedItems.Count == 0)
                return;
            Profiler.SampleCount s = listView1.SelectedItems[0].Tag as Profiler.SampleCount;
            int line = _profiler.AddressToLine(s.Addr);
            vScrollBar1.Value = (line - 1) * _lineHeight;
        }

        private void panel1_Paint(object sender, PaintEventArgs e)
        {
            if (!_profiler.IsOpen())
                return;

            int from = e.ClipRectangle.Top + vScrollBar1.Value;
            int to = e.ClipRectangle.Bottom + vScrollBar1.Value;
            from /= _lineHeight;
            to /= _lineHeight;

            int y = from * _lineHeight - vScrollBar1.Value + 2;
            int x = 8;
            int width = panel1.Width - 16;
            while (from < to)
            {
                int addr = _profiler.LineToAddress(from);
                if (addr != -1)
                {
                    int w = width * _profiler.GetSample(addr);
                    if (_profiler.MaxCount == 0)
                        w = 0;
                    else
                        w /= _profiler.MaxCount;
                    e.Graphics.FillRectangle(Brushes.Red, 2, y + 1, w, _lineHeight - 4);
                }

                string s = _profiler.GetLine(from++);
                e.Graphics.DrawString(s, _font, Brushes.Black, x, y);
                y += _lineHeight;
            }
        }

        private void clearSamplesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            _sampleCount = 0;
            _profiler.Clear();
            Update();
        }

        private void fIleToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }

        private void openMapFileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog fd = new OpenFileDialog();
            fd.Filter = "Map Files(*.map)|*.map|All files (*.*)|*.*";
            if (fd.ShowDialog() != DialogResult.OK)
                return;
            Form2 pie = new Form2();
            pie.SetMapFile(fd.FileName);
            pie.Show();
        }
    }
}
