using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

namespace MicrotouchProfiler
{
    class Datum
    {
        public string Name;
        public double Value;
        public Brush Brush;
    }

    class Piechart
    {
        double _total;
        List<Datum> _values = new List<Datum>();
        Random _r = new Random();
        string _title;

        public Piechart(string title)
        {
            _title = title;
        }

        int Rand8()
        {
            return _r.Next(255);
        }

        Brush RandomBrush(string name)
        {
            int h = name.GetHashCode();
            return new SolidBrush(Color.FromArgb((byte)h,(byte)(h >> 8),(byte)(h>>16)));
        }

        public void Add(string name, double value)
        {
            _values.Add(new Datum() { Name = name, Value = value, Brush = RandomBrush(name) });
            _total += value;
        }

        float ToAngle(double v)
        {
            return (float)(360* v / _total);
        }

        Brush GetBrush(Datum d)
        {
            return d.Brush;
        }

        float ToAngle(Rectangle rect, int x, int y)
        {
            double dx = x -(rect.Left + rect.Width / 2);
            double dy = y - (rect.Top + rect.Height / 2);
            double hilite = Math.Atan2(dy,dx);
            if (hilite < 0)
                hilite += 2 * Math.PI;
            return (float)(hilite * 360 / (2 * Math.PI));
        }

        Font _font = new Font("Arial", 10);
        void DrawTitle(Graphics g, Rectangle rect, Datum d)
        {
            int x = rect.Right + 16;
            int y = rect.Top;

            g.DrawString(_title + " " + _total, _font, Brushes.DarkGray, x, y);
            y += 20;
            g.DrawString(d.Name, _font, Brushes.DarkGray, x, y);
            y += 20;
            g.DrawString(d.Value.ToString() + " (" + (int)(d.Value*100/_total)+ "%)", _font, Brushes.DarkGray, x, y);
        }

        int BySize(Datum x, Datum y)
        {
            if (x.Value > y.Value)
                return 1;
            if (x.Value == y.Value)
                return String.CompareOrdinal(x.Name, y.Name);
            return -1;
        }

        public Datum Draw(Graphics g, Rectangle rect, int x, int y)
        {
            _values.Sort(BySize);

            float hilite = ToAngle(rect, x, y);
            double last = 0;
            Datum hilited = null;
            foreach (Datum d in _values)
            {
                double next = last + d.Value;
                float angle = ToAngle(last);
                float sweep = ToAngle(next-last);
                if (hilite >= angle && hilite < (angle + sweep))
                {
                    hilited = d;
                    g.FillPie(Brushes.Red, rect, angle, sweep);
                    DrawTitle(g,rect,d);
                }
                else
                    g.FillPie(GetBrush(d), rect, angle, sweep);
                last = next;
            }
            return hilited;
        }
    }
}
