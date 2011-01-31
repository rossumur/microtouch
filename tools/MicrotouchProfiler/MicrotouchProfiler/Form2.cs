using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MicrotouchProfiler
{
    public partial class Form2 : Form
    {
        public Form2()
        {
            InitializeComponent();
        }

        MapFile _map;
        Piechart _pie;
        int _x = 200;
        int _y = 200;
        int _radius = 190;
        bool _detail;
        int _mx;
        int _my;
        Datum _selected = null;

        void SetPie(MapFile map)
        {
            _pie = new Piechart(map.Name);
            foreach (ObjectFile obj in map.Values)
                _pie.Add(obj.Name, obj.TotalSize);
            panel1.Invalidate();
        }

        void SetPie(MapFile map, string name)
        {
            _pie = new Piechart(name);
            ObjectFile obj = map[name];
            foreach (Blob blob in obj)
                _pie.Add(blob.Name, blob.Size);
            panel1.Invalidate();
        }

        public void SetMapFile(string path)
        {
            MapGrinder g = new MapGrinder();
            this.TopLevelControl.Text = path;
            _map = g.Grind(path);
            SetPie(_map);
        }

        private void panel1_Paint(object sender, PaintEventArgs e)
        {
            if (_pie == null)
                return;
            Rectangle rect = new Rectangle(_x - _radius, _y - _radius, _x + _radius, _y + _radius);
            _selected = _pie.Draw(e.Graphics, rect, _mx, _my);
        }

        private void panel1_MouseMove(object sender, MouseEventArgs e)
        {
            _mx = e.X;
            _my = e.Y;
            panel1.Invalidate();
        }

        class PiePanel : Panel
        {
            public PiePanel()
            {
                DoubleBuffered = true;
            }
        }

        private void panel1_Click(object sender, EventArgs e)
        {
            if (_selected == null)
                return;
            _detail = !_detail;
            if (_detail)
                SetPie(_map, _selected.Name);
            else
                SetPie(_map);
        }
    }
}
