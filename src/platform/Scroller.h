
typedef void (*ScrollDrawProc)(long scroll, int y, int height, void* ref);

//  Encapsulate fancy scrolling
class Scroller
{
public:

protected:
    long _scroll;			//todo will need to be long for >100 pages
    short _scrollHeight;
    short _dragy;
    short _velocity;
	short _pageSize;

	ScrollDrawProc _drawProc;
	void* _ref;
    
public:
	long CurrentScroll() { return _scroll; }
	void Init(long height, ScrollDrawProc drawProc, void* ref, int pageSize = 0);
	void Clear(int color = 0xFFFF);
	int OnEvent(Event* e);
    void ScrollTo(int delta);
	void SetHeight(long height);
	bool Stopped();

protected:
	void DrawBody(int y, int height);
	int Acceleration();
    void ScrollBy(int delta);
	void Invalidate(long src, int lines);
    void AutoScroll();
};
