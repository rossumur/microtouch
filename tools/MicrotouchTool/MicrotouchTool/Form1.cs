using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.IO;
using System.Windows.Forms;

namespace MicrotouchTool
{
    public partial class Form1 : Form
    {
        ContextMenu _menu;
        Color _bgcolor;
        bool _fit = true;
        int _zoom = 100;
        bool _portrait = false;
        int _width = 320;
        int _height = 240;

        public Form1()
        {
            MenuItem mi = new MenuItem();
            InitializeComponent();
            _menu = new ContextMenu(new MenuItem[] {
                new MenuItem("Remove",OnRemove),
                new MenuItem("Fit Width",OnFitWidth),
                new MenuItem("Fit Height",OnFitHeight),
                new MenuItem("Set Background Color",OnColor)
            });
            _bgcolor = Color.DarkGray;
            saveIM2ToolStripMenuItem.Enabled = false;
            landscapeToolStripMenuItem.Checked = true;
        }

        class PictureTag
        {
            public string Path;
            public Color Background;
            public bool FitHeight = true;
        }

        private void OnRemove(object sender, EventArgs e)
        {
            PictureBox source = _menu.SourceControl as PictureBox;
            this.flowLayoutPanel1.Controls.Remove(source);
            updateMenu();
        }

        private void OnFitWidth(object sender, EventArgs e)
        {
            PictureBox source = _menu.SourceControl as PictureBox;
            PictureTag t = source.Tag as PictureTag;
            if (t.FitHeight)
            {
                t.FitHeight = false;
                source.Image = MakeImage(t);
            }
            _fit = false;
        }
        private void OnFitHeight(object sender, EventArgs e)
        {
            PictureBox source = _menu.SourceControl as PictureBox;
            PictureTag t = source.Tag as PictureTag;
            if (!t.FitHeight)
            {
                t.FitHeight = true;
                source.Image = MakeImage(t);
            }
            _fit = true;
        }

        private void OnColor(object sender, EventArgs e)
        {
            PictureBox source = _menu.SourceControl as PictureBox;
            PictureTag t = source.Tag as PictureTag;

            ColorDialog d = new ColorDialog();
            d.Color = t.Background;
            if (d.ShowDialog() == DialogResult.OK)
            {
                _bgcolor = t.Background = d.Color;
                source.Image = MakeImage(t);
            }
        }

        Image MakeImage(PictureTag t)
        {
            Image img = Image.FromFile(t.Path);

            int sourceWidth = img.Width;
            int sourceHeight = img.Height;

            int width = _width;
            int height = _height;

            // Fit height or fit width
            if (t.FitHeight)
            {
                width = img.Width * height / img.Height;    // Fit height
            }
            else
            {
                height = img.Height * width / img.Width;    // Fit width
            }

            Bitmap bm = new Bitmap(_width, _height);
            Graphics g = Graphics.FromImage(bm);
            g.FillRectangle(new SolidBrush(t.Background), new Rectangle(0, 0, _width, _height));
            g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;
            Rectangle r = new Rectangle((_width - width)/2,(_height - height)/2,width,height);
            g.DrawImage(img,r);
            g.Dispose();
            return bm;
        }

        void updateMenu()
        {
            saveIM2ToolStripMenuItem.Enabled = flowLayoutPanel1.Controls.Count != 0;
        }

        void openImage()
        {
            OpenFileDialog fd = new OpenFileDialog();
            fd.Filter = "Image Files(*.bmp;*.jpg;*.png;*.gif)|*.bmp;*.jpg;*.png;*.gif|All files (*.*)|*.*";
            fd.Multiselect = true;

            if (fd.ShowDialog() == DialogResult.OK)
            {
                for (int c = 0; c < fd.FileNames.Length; c++)
                {
                    string path = fd.FileNames[c].ToString();

                    PictureBox p1 = new PictureBox();
                    p1.BorderStyle = BorderStyle.FixedSingle;
                    p1.Dock = DockStyle.Top;
                    p1.SizeMode = PictureBoxSizeMode.StretchImage;
                    p1.Width = _width * _zoom / 100;
                    p1.Height = _height * _zoom / 100;
                    p1.Tag = new PictureTag() { Background = _bgcolor, Path = path, FitHeight = _fit };
                    p1.Image = MakeImage(p1.Tag as PictureTag);

                    p1.AllowDrop = true;
                    p1.MouseMove += OnPictureMouseMove;
                    p1.DragDrop += OnPictureDragDrop;
                    p1.DragEnter += OnPictureDragEnter;
                    p1.MouseUp += OnPictureMouseUp;
                    p1.DragLeave += OnPictureDragLeave;
                    p1.ContextMenu = _menu;
                    flowLayoutPanel1.Controls.Add(p1);
                }
            }
            updateMenu();
        }

        private void OnPictureMouseUp(object sender, MouseEventArgs e)
        {
        }

        private void OnPictureDragLeave(object sender, EventArgs e)
        {
        }

        private void OnPictureDragEnter(object sender, DragEventArgs e)
        {
            e.Effect = DragDropEffects.Move;
        }

        private void OnPictureDragDrop(object sender, DragEventArgs e)
        {
            PictureBox target = sender as PictureBox;
            if (target != null)
            {
                int targetIndex = FindPictureBoxIndex(target);
                if (targetIndex != -1)
                {
                    string pictureBoxFormat = typeof(PictureBox).FullName;
                    if (e.Data.GetDataPresent(pictureBoxFormat))
                    {
                        PictureBox source = e.Data.GetData(pictureBoxFormat) as PictureBox;
                        int sourceIndex = this.FindPictureBoxIndex(source);
                        if (targetIndex != -1)
                            this.flowLayoutPanel1.Controls.SetChildIndex(source, targetIndex);
                    }
                }
            }
        }

        private int FindPictureBoxIndex(PictureBox picture)
        {
            for (int i = 0; i < this.flowLayoutPanel1.Controls.Count; i++)
            {
                PictureBox target = this.flowLayoutPanel1.Controls[i] as PictureBox;
                if (picture == target)
                    return i;
            }
            return -1;
        }

        private void OnPictureMouseMove(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                PictureBox picture = sender as PictureBox;
                picture.DoDragDrop(picture, DragDropEffects.Move);
            }
        }

        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            openImage();
        }

        void Refresh(bool reload)
        {
            foreach (Control c in this.flowLayoutPanel1.Controls)
            {
                PictureBox p = c as PictureBox;
                p.Width = _width * _zoom / 100;
                p.Height = _height * _zoom / 100;
                if (reload)
                    p.Image = MakeImage(p.Tag as PictureTag);
            }
        }

        void Zoom(int zoom)
        {
            _zoom = zoom;
            Refresh(false);
        }

        private void toolStripMenuItem2_Click(object sender, EventArgs e)
        {
            Zoom(25);
        }

        private void toolStripMenuItem3_Click(object sender, EventArgs e)
        {
            Zoom(50);
        }

        private void toolStripMenuItem4_Click(object sender, EventArgs e)
        {
            Zoom(100);
        }

        private void toolStripMenuItem5_Click(object sender, EventArgs e)
        {
            Zoom(200);
        }

        private void fileToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }

        private void saveIM2ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SaveFileDialog fd = new SaveFileDialog();
            fd.Filter = "IM2 files (*.im2)|*.im2|All files (*.*)|*.*";
            if (fd.ShowDialog() == DialogResult.OK)
            {
                Stream s;
                if ((s = fd.OpenFile()) != null)
                {
                    IM2.WriteHeader(s,this.flowLayoutPanel1.Controls.Count);
                    foreach (Control c in this.flowLayoutPanel1.Controls)
                    {
                        PictureBox p = c as PictureBox;
                        IM2.WriteRaw(s, p.Image as Bitmap);
                    }
                    s.Close();
                }
            }
        }

        void SetAspect(bool portrait)
        {
            if (portrait == _portrait)
                return;
            _portrait = portrait;
            portraitToolStripMenuItem.Checked = _portrait;
            landscapeToolStripMenuItem.Checked = !_portrait;
            if (_portrait)
            {
                _width = 240;
                _height = 320;
            }
            else
            {
                _width = 320;
                _height = 240;
            }
            Refresh(true);
        }

        private void portraitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SetAspect(true);
        }

        private void landscapeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SetAspect(false);
        }
    }
}
