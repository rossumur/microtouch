
/* Copyright (c) 2009, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
**  
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#include "Platform.h"
#include "PacmanAppTiles.h"

extern const byte _initSprites[] PROGMEM;
extern const byte _palette2[] PROGMEM;
extern const byte _paletteIcon2[] PROGMEM;

extern const byte _opposite[] PROGMEM;
extern const byte _scatterChase[] PROGMEM;
extern const byte _scatterTargets[] PROGMEM;
extern const char _pinkyTargetOffset[] PROGMEM;

extern const byte _pacLeftAnim[] PROGMEM;
extern const byte _pacRightAnim[] PROGMEM;
extern const byte _pacVAnim[] PROGMEM;


enum GameState {
    ReadyState,
    PlayState,
    DeadGhostState, // Player got a ghost, show score sprite and only move eyes
    DeadPlayerState,
    EndLevelState
};

enum SpriteState
{
    PenState,
    RunState,
    FrightenedState,
    DeadNumberState,
    DeadEyesState,
    AteDotState,    // pacman
    DeadPacmanState
};

enum {
    MStopped = 0,
    MRight = 1,
    MDown = 2,
    MLeft = 3,
    MUp = 4,
};
    
#define C16 TOCOLOR

//  8 bit palette - in RAM because of graphics driver
short _paletteW[] = 
{
    C16(0,0,0),
    C16(255,0,0),        // 1 red
    C16(222,151,81),     // 2 brown
    C16(255,184,255),    // 3 pink

    C16(0,0,0),
    C16(0,255,255),      // 5 cyan
    C16(71,84,255),      // 6 mid blue
    C16(255,184,81),     // 7 lt brown

    C16(0,0,0),
    C16(255,255,0),      // 9 yellow
    C16(0,0,0),
    C16(33,33,255),      // 11 blue
    
    C16(0,255,0),        // 12 green
    C16(71,84,174),      // 13 aqua
    C16(255,184,174),    // 14 lt pink
    C16(222,222,255),    // 15 whiteish
};
byte _swizzled = 0;

#define BINKY 0
#define PINKY 1
#define INKY  2
#define CLYDE 3
#define PACMAN 4

const byte _initSprites[] = 
{
    BINKY,  14,     17-3,   31, MLeft,
    PINKY,  14-2,   17,     79, MLeft,
    INKY,   14,     17,     137, MLeft,
    CLYDE,  14+2,   17,     203, MRight,
    PACMAN, 14,     17+9,     0, MLeft,
};

//  Ghost colors
const byte _palette2[] = 
{
    0,11,1,15, // BINKY red
    0,11,3,15, // PINKY pink
    0,11,5,15, // INKY cyan
    0,11,7,15, // CLYDE brown
    0,11,9,9,  // PACMAN yellow
    0,11,15,15,// FRIGHTENED
    0,11,0,15, // DEADEYES
};

const byte _paletteIcon2[] = 
{
    0,9,9,9,    // PACMAN
    
    0,2,15,1,   // cherry
    0,12,15,1,  // strawberry
    0,12,2,7,   // peach
    0,5,15,9,   // bell
    
    0,2,15,1,   // apple
    0,12,15,5,  // grape
    0,1,9,11,   // galaxian
    0,5,15,15,  // key
};

#define PACMANICON 1

#define FRIGHTENEDPALETTE 5
#define DEADEYESPALETTE 6

#define FPS 60
#define CHASE 0
#define SCATTER 1
   
#define DOT 7
#define PILL 14
#define PENGATE 0x1B

const byte _opposite[] = { MStopped,MLeft,MUp,MRight,MDown };
#define OppositeDirection(_x) pgm_read_byte(_opposite + _x)

const byte _scatterChase[] = { 7,20,7,20,5,20,5,0 };
const byte _scatterTargets[] = { 2,0,25,0,0,35,27,35 }; // inky/clyde scatter targets are backwards
const char _pinkyTargetOffset[] = { 4,0,0,4,-4,0,-4,4 }; // Includes pinky target bug

#define FRIGHTENEDGHOSTSPRITE 0
#define GHOSTSPRITE 2
#define NUMBERSPRITE 10
#define PACMANSPRITE 14

const byte _pacLeftAnim[] = { 5,6,5,4 };
const byte _pacRightAnim[] = { 2,0,2,4 };
const byte _pacVAnim[] = { 4,3,1,3 };
     
class Sprite
{
public:
    short _x,_y;
    short lastx,lasty;
    byte cx,cy;         // cell x and y
    byte tx,ty;         // target x and y
    
    SpriteState state;
    byte  pentimer;     // could be the same
    
    byte who;
    byte speed;
    byte dir;
    byte phase;

    // Sprite bits
    byte palette2;  // 4->16 color map index
    byte bits;      // index of sprite bits
    char sy;
    
    void Init(const byte* s)
    {
        who = pgm_read_byte(s++);
        cx =  pgm_read_byte(s++);
        cy =  pgm_read_byte(s++);
        pentimer = pgm_read_byte(s++);
        dir = pgm_read_byte(s);
        _x = lastx = (short)cx*8-4;
        _y = lasty = (short)cy*8;
        state = PenState;
        speed = 0;
    }
    
    void Target(byte x, byte y)
    {
        tx = x;
        ty = y;
    }
    
    short Distance(byte x, byte y)
    {
        short dx = cx - x;
        short dy = cy - y;
        return dx*dx + dy*dy;   // Distance to target
    }
        
    //  once per sprite, not 9 times
    void SetupDraw(GameState gameState, byte deadGhostIndex)
    {
        sy = 1;
        palette2 = who;
        byte p = phase >> 3;
        if (who != PACMAN)
        {
            bits = GHOSTSPRITE + ((dir-1) << 1) + (p&1);  // Ghosts
            switch (state)
            {
                case FrightenedState:
                    bits = FRIGHTENEDGHOSTSPRITE + (p&1);  // frightened
                    palette2 = FRIGHTENEDPALETTE;
                    break;
                case DeadNumberState:
                    palette2 = FRIGHTENEDPALETTE;
                    bits = NUMBERSPRITE+ deadGhostIndex;
                    break;
                case DeadEyesState:
                    palette2 = DEADEYESPALETTE;
                    break;
                default:
                    ;
            }
            return;
        }
        
        //  PACMAN animation
        byte f = (phase>>1) & 3;
        if (dir == MLeft)
            f = pgm_read_byte(_pacLeftAnim + f);
        else if (dir == MRight)
             f = pgm_read_byte(_pacRightAnim + f);
        else
            f = pgm_read_byte(_pacVAnim + f);
        if (dir == MUp)
            sy = -1;
        bits = f + PACMANSPRITE;
    }
         
    //  Draw this sprite into the tile at x,y
    void Draw8(short x, short y, byte* tile)
    {
        short px = x - (_x-4);
        if (px <= -8 || px >= 16) return;
        short py = y - (_y-4);
        if (py <= -8 || py >= 16) return;
    
        // Clip y
        short lines = py+8;
        if (lines > 16)
            lines = 16;
        if (py < 0)
        {
            tile -= py*8;
            py = 0;
        }
        lines -= py;
            
        //  Clip in X
        byte right = 16 - px;
        if (right > 8)
            right = 8;
        byte left = 0;
        if (px < 0)
        {
            left = -px;
            px = 0;
        }
                    
        //  Get bitmap
        char dy = sy;
        if (dy < 0)
            py = 15-py;    // VFlip
        byte* data = (byte*)(pacman16x16+bits*64);
        data += py << 2;
        dy <<= 2;  
        data += px >> 2;
        px &= 3;
        
        const byte* palette = _palette2 + (palette2<<2);
        while (lines)
        {
            const byte *src = data;
            byte d = pgm_read_byte(src++);
            d >>= px << 1;
            byte sx = 4 - px;
            byte x = left;
            do
            {
                byte p = d & 3;         
                if (p)
                {
                    p = pgm_read_byte(palette+p);
                    if (p)
                        tile[x] = p;
                }
                d >>= 2;    // Next pixel
                if (!--sx)
                {
                    d = pgm_read_byte(src++);
                    sx = 4;
                }
            } while (++x < right);
            
            tile += 8;
            data += dy;
            lines--;
        }
    }
};


class Playfield
{
    Sprite _sprites[5];
    byte _dotMap[(32/4)*(36-6)];
    
    GameState _state;
    long    _score;             // 7 digits of score
    char    _scoreStr[8];
    byte    _icons[14];         // Along bottom of screen
    
    ushort  _stateTimer;
    ushort  _frightenedTimer;
    byte    _frightenedCount;
    byte    _scIndex;           //
    ushort  _scTimer;           // next change of sc status  
    
   // bool _inited;
    byte* _dirty;

	byte _ptx;
	byte _pty;
    
public:
    // Draw 2 bit BG into 8 bit icon tiles at bottom
    void DrawBG2(byte cx, byte cy, byte* tile)
    {
        byte index = _icons[cx >> 1];   // 13 icons across bottom
        if (index == 0)
        {
            memset(tile,0,64);
            return;
        }
        index--;
        
        byte b = (1-(cx&1)) + ((cy&1)<<1);  // Index of tile
        index <<= 2;                        // 4 tiles per icon
        const byte* bg = pacman8x8x2 + ((b + index) << 4);
        const byte* palette = _paletteIcon2 + index;
        
        byte x = 16;
        while (x--)
        {
            byte bits = (char)pgm_read_byte(bg++);
            byte i = 4;
            while (i--)
            {
                tile[i] = pgm_read_byte(palette + (bits & 3));
                bits >>= 2;
            }
            tile += 4;
        }
    }
    
    // Draw 1 bit BG into 8 bit tile
    void DrawBG(byte cx, byte cy, byte* tile)
    {
        if (cy >= 34)
        {
            DrawBG2(cx,cy,tile);
            return;
        }
        
        byte c = 11;            // Blue
        byte b = GetTile(cx,cy);
        const byte* bg;
        
        //  This is a little messy
        memset(tile,0,64);
        if (cy == 20 && cx >= 11 && cx < 17)
        {
            if (_state != ReadyState)
                b = 0;  // hide 'READY!'
        }
        else if (cy == 1)
        {
            if (cx < 7)
                b = _scoreStr[cx];
            else if (cx >= 10 && cx < 17)
                b = _scoreStr[cx-10];
        } else {
            if (b == DOT || b == PILL)
            {
                if (!GetDot(cx,cy))
                    return;
                c = 14;
            }
            if (b == PENGATE)
                c = 14;
        }
        bg = playTiles + (b << 3);
        if (b >= '0')
            c = 15; // text is white
            
        for (byte y = 0; y < 8; y++)
        {
            signed char bits = (signed char)pgm_read_byte(bg++);
            byte x = 0;
            while (bits)
            {
                if (bits < 0)
                    tile[x] = c;
                bits <<= 1;
                x++;
            }
            tile += 8;
        }
    }
       
    // Draw BG then all sprites in this cell
    void Draw(short x, short y, bool sprites)
    {
        byte tile[8*8];
        
//      Fill with BG
        DrawBG(x,y,tile);
        
//      Overlay sprites
        x <<= 3;
        y <<= 3;
        if (sprites)
        {
            for (byte i = 0; i < 5; i++)
                _sprites[i].Draw8(x,y,tile);
        }

//      Show sprite block
        #if 0
        for (byte i = 0; i < 5; i++)
        {
            Sprite* s = _sprites + i;
            if (s->cx == (x>>3) && s->cy == (y>>3))
            {
                memset(tile,0,8);
                for (byte j = 1; j < 7; j++)
                    tile[j*8] = tile[j*8+7] = 0;
                memset(tile+56,0,8);
            }
        }
        #endif
        
        x += (240-224)/2;
        y += (320-288)/2;
        
//      Should be a direct Graphics call
        Graphics.SetBounds(x,y,8,8);
        Graphics.BlitIndexed(tile,(byte*)_paletteW,64);
    }
    
    //  Mark tile as dirty (should not need range checking here)
    static void Mark(short x, short y, byte* m)
    {
        x -= 4;
        y -= 4;
        short top = y >> 3;
        short bottom = ((y + 16 + 7) >> 3);
        top = max(0,top);
        bottom = min(36,bottom);
        
        byte* row = m + (top << 2);  // 32 bits per row
        while (top < bottom)
        {
            short left = x >> 3;
            short right = (x + 16 + 7) >> 3;
            left = max(0,left);
            right = min(28,right);
            while (left < right)
            {
                row[left >> 3] |= 0x80 >> (left & 7);
                left++;
            }
            row += 4;
            top++;
        }
    }
 
    void DrawAllBG()
    {            
        for (byte y = 0; y < 36; y++)
        for (byte x = 0; x < 28; x++)
            Draw(x,y,false);
    }
    
    //  Draw sprites overlayed on cells
    //  I love sprites
    void DrawAll()
    {
        byte* m = _dirty;

        //  Mark sprite old/new positions as dirty
        for (byte i = 0; i < 5; i++)
        {
            Sprite* s = _sprites + i;
            Mark(s->lastx,s->lasty,m);
            Mark(s->_x,s->_y,m);
        }
        
        //  Animation
        for (byte i = 0; i < 5; i++)
            _sprites[i].SetupDraw(_state,_frightenedCount-1);
    
        //  Redraw only dirty tiles
        byte* row = m;
        for (byte y = 0; y < 36; y++)   // skip n lines TODO
        {
            for (byte x = 0; x < 32; x += 8)    // 28 actually
            {
                char b = (char)*row++;
                byte xx = x;
                while (b)
                {
                    if (b < 0)
                        Draw(xx,y,true);
                    b <<= 1;
                    xx++;
                }
            }
        }
    }
    
    byte GetTile(int cx, int ty)
    {
        return pgm_read_byte(playMap + ty*28 + cx);
    }
        
    short Chase(Sprite* s, short cx, short cy)
    {
        while (cx < 0)      //  Tunneling
            cx += 28;
        while (cx >= 28)
            cx -= 28;
    
        byte t = GetTile(cx,cy);
        if (!(t == 0 || t == DOT || t == PILL || t == PENGATE))
            return 0x7FFF;
            
        if (t == PENGATE)
        {
            if (s->who == PACMAN)
                return 0x7FFF;  // Pacman can't cross this to enter pen
            if (!(InPen(s->cx,s->cy) || s->state == DeadEyesState))
                return 0x7FFF;  // Can cross if dead or in pen trying to get out
        }
        
        short dx = s->tx-cx;
        short dy = s->ty-cy;
        return dx*dx + dy*dy;   // Distance to target
    }
    
    void UpdateTimers()
    {
        // Update scatter/chase selector, low bit of index indicates scatter
        if (_scIndex < 8)
        {
            if (_scTimer-- == 0)
            {
                byte duration = pgm_read_byte(_scatterChase + _scIndex++);
                _scTimer = duration*FPS;
            }
        }
        
        //  Release frightened ghosts
        if (_frightenedTimer && !--_frightenedTimer)
        {
            for (byte i = 0; i < 4; i++)
            {
                Sprite* s = _sprites + i;
                if (s->state == FrightenedState)
                {
                    s->state = RunState;
                    s->dir = OppositeDirection(s->dir);
                }
            }
        }
    }
    
    //  Target closes pill, run from ghosts?
    void PacmanAI()
    {
        Sprite* pacman;
        pacman = _sprites + PACMAN;
        
        //  Chase frightened ghosts
        Sprite* closestGhost = NULL;
        Sprite* frightenedGhost = NULL;
        short dist = 0x7FFF;
        for (byte i = 0; i < 4; i++)
        {
            Sprite* s = _sprites+i;
            short d = s->Distance(pacman->cx,pacman->cy);
            if (d < dist)
            {
                dist = d;
                if (s->state == FrightenedState)
                    frightenedGhost = s;
                closestGhost = s;
            }
        }
        if (frightenedGhost)
        {
            pacman->Target(frightenedGhost->cx,frightenedGhost->cy);
            return;
        }
        
        // Under threat; just avoid closest ghost
        if (dist < 16)
        {
            pacman->Target(pacman->cx*2 - closestGhost->cx,pacman->cy*2 - closestGhost->cy);
            return;
        }
        
        //  Go for the pill
        if (GetDot(1,6))
            pacman->Target(1,6);
        else if (GetDot(26,6))
            pacman->Target(26,6);
        else if (GetDot(1,26))
            pacman->Target(1,26);
        else if (GetDot(26,26))
            pacman->Target(26,26);
        else
        {
            // closest dot
            short dist = 0x7FFF;
            for (byte y = 4; y < 32; y++)
            {
                for (byte x = 1; x < 26; x++)
                {
                    if (GetDot(x,y))
                    {
                        short d = pacman->Distance(x,y);
                        if (d < dist)
                        {
                            dist = d;
                            pacman->Target(x,y);
                        }
                    }
                }
            }
            
            if (dist == 0x7FFF) // No dots, just avoid closest ghost
                pacman->Target(pacman->cx*2 - closestGhost->cx,pacman->cy*2 - closestGhost->cy);
        }
    }
    
    void Scatter(Sprite* s)
    {
        const byte* st = _scatterTargets + (s->who << 1);
        s->Target(pgm_read_byte(st),pgm_read_byte(st+1));
    }
    
    void UpdateTargets()
    {
        if (_state == ReadyState)
            return;
        Sprite* pacman = _sprites + PACMAN;

		if (_ptx == 0xFF)
			PacmanAI();
		else
			pacman->Target(_ptx,_pty);	// Send him somewhere
       
        //  Ghost AI
        bool scatter = _scIndex & 1;
        for (byte i = 0; i < 4; i++)
        {
            Sprite* s = _sprites+i;
            
            //  Deal with returning ghost to pen
            if (s->state == DeadEyesState)
            {
                if (s->cx == 14 && s->cy == 17) // returned to pen
                {
                    s->state = PenState;        // Revived in pen
                    s->pentimer = 80;
                }
                else
                    s->Target(14,17);           // target pen
                continue;           // 
            }
            
            //  Release ghost from pen when timer expires
            if (s->pentimer)
            {
                if (--s->pentimer)  // stay in pen for awhile
                    continue;
                s->state = RunState;
            }
            
            if (InPen(s->cx,s->cy))
            {
                s->Target(14,14-2); // Get out of pen first
            } else {
                if (scatter || s->state == FrightenedState)
                    Scatter(s);
                else
                {
                    // Chase mode targeting
                    byte tx = pacman->cx;
                    byte ty = pacman->cy;
                    switch (s->who)
                    {
                        case PINKY:
                            {
                                const char* pto = _pinkyTargetOffset + ((pacman->dir-1)<<1);
                                tx += pgm_read_byte(pto);
                                ty += pgm_read_byte(pto+1);
                            }
                            break;
                        case INKY:
                            {
                                const char* pto = _pinkyTargetOffset + ((pacman->dir-1)<<1);
                                Sprite* binky = _sprites + BINKY;
                                tx += pgm_read_byte(pto)>>1;
                                ty += pgm_read_byte(pto+1)>>1;
                                tx += tx - binky->cx;
                                ty += ty - binky->cy;
                            }
                            break;
                        case CLYDE:
                            {
                                if (s->Distance(pacman->cx,pacman->cy) < 64)
                                {
                                    const byte* st = _scatterTargets + CLYDE*2;
                                    tx = pgm_read_byte(st);
                                    ty = pgm_read_byte(st+1);
                                }
                            }
                            break;
                    }
                    s->Target(tx,ty);
                }
            }
        }
    }
    
    //  Default to current direction
    byte ChooseDir(int dir, Sprite* s)
    {
        short choice[4];
        choice[0] = Chase(s,s->cx,s->cy-1);   // Up
        choice[1] = Chase(s,s->cx-1,s->cy);   // Left
        choice[2] = Chase(s,s->cx,s->cy+1);   // Down
        choice[3] = Chase(s,s->cx+1,s->cy);   // Right
        
        // Don't choose opposite of current direction?
        
        short dist = choice[4-dir]; // favor current direction
        byte opposite = OppositeDirection(dir);
        for (byte i = 0; i < 4; i++)    
        {
            byte d = 4-i;
            if (d != opposite && choice[i] < dist)
            {
                dist = choice[i];
                dir = d;
            }
        }
        return dir;
    }
    
    bool InPen(byte cx, byte cy)
    {
        if (cx <= 10 || cx >= 18) return false;
        if (cy <= 14 || cy >= 18) return false;
            return true;
    }
    
    byte GetSpeed(Sprite* s)
    {
        if (s->who == PACMAN)
            return _frightenedTimer ? 90 : 80;
        if (s->state == FrightenedState)
            return 40;
        if (s->state == DeadEyesState)
            return 100;
        if (s->cy == 17 && (s->cx <= 5 || s->cx > 20))
            return 40;  // tunnel
        return 75;
    }
    
    void MoveAll()
    {
        UpdateTimers();
        UpdateTargets();
        
        //  Update game state
        if (_stateTimer)
        {
            if (--_stateTimer == 0)
            {
                switch (_state)
                {
                    case ReadyState:
                        _state = PlayState;
                        _dirty[20*4 + 1] |= 0x1F;  // Clear 'READY!'
                        _dirty[20*4 + 2] |= 0x80;
                        break;
                    case DeadGhostState:
                        _state = PlayState;
                        for (byte i = 0; i < 4; i++)
                        {
                            Sprite* s = _sprites + i;
                            if (s->state == DeadNumberState)
                                s->state = DeadEyesState;
                        }
                        break;
                    default:
                        ;
                }
            } else {
                if (_state == ReadyState)
                    return;
            }
        }
        
        for (byte i = 0; i < 5; i++)
        {
            Sprite* s = _sprites + i;
            
            //  In DeadGhostState, only eyes move
            if (_state == DeadGhostState && s->state != DeadEyesState)
                continue;
            
            //  Calculate speed
            s->speed += GetSpeed(s);
            if (s->speed < 100)
                continue;
            s->speed -= 100;
            
            s->lastx = s->_x;
            s->lasty = s->_y;
            s->phase++;
            
            int x = s->_x;
            int y = s->_y;
            
            if ((x & 0x7) == 0 && (y & 0x7) == 0)   // cell aligned
                s->dir = ChooseDir(s->dir,s);       // time to choose another direction
            
            switch (s->dir)
            {
                case MLeft:     x -= 1; break;
                case MRight:    x += 1; break;
                case MUp:       y -= 1; break;
                case MDown:     y += 1; break;
            }
            
            //  Wrap x because of tunnels
            while (x < 0)
                x += 224;
            while (x >= 224)
                x -= 224;
                
            s->_x = x;
            s->_y = y;
            s->cx = (x + 4) >> 3;
            s->cy = (y + 4) >> 3;
            
            if (s->who == PACMAN)
                EatDot(s->cx,s->cy);
        }
        
        //  Collide
        Sprite* pacman = _sprites + PACMAN;
        for (byte i = 0; i < 4; i++)
        {
            Sprite* s = _sprites + i;
            if (s->cx == pacman->cx && s->cy == pacman->cy)
            {
                if (s->state == FrightenedState)
                {
                    s->state = DeadNumberState;     // Killed a ghost
                    _frightenedCount++;
                    _state = DeadGhostState;
                    _stateTimer = 2*FPS;
                    Score((1 << _frightenedCount)*100);
                }
                else
                    ;               // pacman died
            }
        }
    }
    
    //  Mark a position dirty
    void Mark(short pos)
    {
        _dirty[pos >> 3] |= 0x80 >> (pos & 7);
    }
    
    void SetScoreChar(byte i, char c)
    {
        if (_scoreStr[i] == c)
            return;
        _scoreStr[i] = c;
        Mark(i+32);
        Mark(i+32+10);
    }
    
    void Score(int delta)
    {
        char str[8];
        _score += delta;
        sprintf(str,"%ld",_score);
        byte i = 7-strlen(str);
        byte j = 0;
        while (i < 7)
            SetScoreChar(i++,str[j++]);
    }
    
    bool GetDot(byte cx, byte cy)
    {
        return _dotMap[(cy-3)*4 + (cx >> 3)] & (0x80 >> (cx & 7));
    }
    
    void EatDot(byte cx, byte cy)
    {
        if (!GetDot(cx,cy))
            return;
        byte mask = 0x80 >> (cx & 7);
        _dotMap[(cy-3)*4 + (cx >> 3)] &= ~mask;
        
        byte t = GetTile(cx,cy);
        if (t == PILL)
        {
            _frightenedTimer = 10*FPS;
            _frightenedCount = 0;
            for (byte i = 0; i < 4; i++)
            {
                Sprite* s = _sprites+i;
                if (s->state == RunState)
                {
                    s->state = FrightenedState;
                    s->dir = OppositeDirection(s->dir);
                }
            }
            Score(50);
        }
        else
            Score(10);
    }
    
    void Init()
    {
        //  Swizzle palette TODO just fix in place
        if (!_swizzled)
        {
            _swizzled = 1;
            byte * p = (byte*)_paletteW;
            for (int i = 0; i < 16; i++)
            {
                ushort w = _paletteW[i];    // Swizzle (palette needs to be in ram for LCD.cpp TODO
                *p++ = w >> 8;
                *p++ = w;
            }
        }

        _state = ReadyState;
        _stateTimer = 3*FPS;
        _frightenedCount = 0;
        _frightenedTimer = 0;
        
        const byte* s = _initSprites;
        for (int i = 0; i < 5; i++)
            _sprites[i].Init(s + i*5);

		_ptx = 0xFF;	// pacman initial target defined by AI
       
        _scIndex = 0;
        _scTimer = 1;
        
        _score = 0;
        memset(_scoreStr,0,sizeof(_scoreStr));
        _scoreStr[5] = _scoreStr[6] = '0';
        memset(_icons,0,sizeof(_icons));
        _icons[0] = _icons[1] = _icons[2] = PACMANICON;
        
        //  Init dots from rom
        memset(_dotMap,0,sizeof(_dotMap));
        byte* map = _dotMap;
        for (byte y = 3; y < 36-3; y++) // 30 interior lines
        {
            for (byte x = 0; x < 28; x++)
            {
                byte t = GetTile(x,y);
                if (t == 7 || t == 14)
                {
                    byte s = x&7;
                    map[x>>3] |= (0x80 >> s);
                }
            }
            map += 4;
        }
        Graphics.Rectangle(0,0,240,320,0);
        DrawAllBG();
    }
    
    void Step()
    {            
        // Create a bitmap of dirty tiles
        byte m[(32/8)*36]; // 144 bytes
        memset(m,0,sizeof(m));
        _dirty = m;
        MoveAll();
        DrawAll();
    }

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				Init();
				break;
			case Event::None:
				Step();
				break;
			case Event::TouchDown:
				if (e->Touch->y > 320)
					return -1;
			case Event::TouchMove:
				_ptx = (e->Touch->x - (240-224)/2)/8;	// Target pacman while touching
				_pty = (e->Touch->y - (320-288)/2)/8;
				//printf("target %d %d\n",_ptx,_pty);
				break;
			case Event::TouchUp:
				_ptx = 0xFF;	// Back to AI
				break;

			default:;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method

INSTALL_APP(pacman,Playfield);