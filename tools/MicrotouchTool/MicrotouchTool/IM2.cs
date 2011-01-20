using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;

namespace MicrotouchTool
{
    class IM2
    {
        public static void WriteHeader(Stream s, int count)
        {
            byte[] h = new byte[512];
            h[0] = (byte)'i';
            h[1] = (byte)'m';
            h[2] = (byte)'g';
            h[3] = (byte)'2';
            LE32(h, 4, 24);       // hdrSize
            LE32(h, 8, 240);   // width
            LE32(h, 12,320*count);   // height
            h[16] = 0x80;    // Raw
            /*
           byte sig[4];
           long hdrSize;
           long width;
           long height;
           byte format;
           byte reserved0;
           byte colors;
           byte restartInterval;
           long reserved1;
 */
            s.Write(h, 0, 512);
        }

        static void LE32(byte[] d, int i, int v)
        {
            for (int j = 0; j < 4; j++)
            {
                d[i++] = (byte)v;
                v >>= 8;
            }
        }

        //  Images are stored portrait
        public static void WriteRaw(Stream s, Bitmap b)
        {
            if (b.Width != 240)
            {
                Bitmap br = new Bitmap(b);
                br.RotateFlip(RotateFlipType.Rotate90FlipNone);
                b = br;
            }

            ushort[] p = Load(b,true);
            byte[] pad = new byte[32];   // Raw frames are 256x320 pixels (1 sector per line)
            int i = 0;
                for (int y = 0; y < 320; y++)
                {
                    for (int x = 0; x < 240; x++)
                    {
                        ushort pix = p[i++];
                        s.WriteByte((byte)(pix >> 8));
                        s.WriteByte((byte)pix);
                    }
                    s.Write(pad, 0, 32);
                }
        }

        public static ushort[] Load(Bitmap bm, bool dither)
        {
            ushort[] p = new ushort[bm.Width * bm.Height];
            int i = 0;
            for (int y = 0; y < bm.Height; y++)
            {
                for (int x = 0; x < bm.Width; x++)
                    p[i++] = ToRGB(bm.GetPixel(x, y), x, y, dither);
            }
            return p;
        }

        static byte[] _dither = {
                0,8,2,10,
                12,4,14,6,
                3,11,1,9,
                15,7,13,5
            };

        public static int clip(int x)
        {
            if (x > 0xFF)
                return 0xFF;
            return x;
        }

        public static int alpha(int a, int c)
        {
            return clip((c * a + 0xFF * (0xFF - a)) / 0xFF);
        }

        public static ushort ToRGB(Color c, int x, int y, bool dither)
        {
            int i = _dither[(x & 3) | ((y & 3) << 2)];
            if (!dither)
                i = 0;
            int r = clip(c.R + (i >> 1));
            int g = clip(c.G + (i >> 2));
            int b = clip(c.B + (i >> 1));

            //  premultiply alpha
            if (c.A != 0xFF)
            {
                r = alpha(c.A, r);
                g = alpha(c.A, g);
                b = alpha(c.A, b);
            }
            return (ushort)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
        }
    }
}
