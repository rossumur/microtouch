
typedef void (*ScrollDrawProc)(long scroll, int y, int height, void* ref);

//  Encapsulate fancy scrolling
class Scroller
{
public:
	enum ScrollMode
    {
        None,
        LinearScroll = 129,
        PageScroll = 130
    };
    
protected:
    short _scroll;			//todo will need to be long for >100 pages
    short _scrollHeight;
    short _dragy;
    short _velocity;
    ScrollMode _scrollMode;

	ScrollDrawProc _drawProc;
	void* _ref;
    
public:
	long CurrentScroll() { return _scroll; }
	void Init(long height, ScrollDrawProc drawProc, void* ref, ScrollMode mode = PageScroll);
	void Clear(int color = 0xFFFF);
	int OnEvent(Event* e);
    void ScrollTo(int delta);
	bool Stopped();

protected:
	int Acceleration();
    void ScrollBy(int delta);
	void Invalidate(int src, int lines);
    void AutoScroll();
};
