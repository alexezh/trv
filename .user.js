var qc = new TraceCollection();
// var qf = $.view.addfilter(qc, Blue, "quick collection");

// add current line to quick collection
function ql() { qc.addline($.view.currentline); }
$.dotexpressions.add("ql", ql);

// Trace line: [2012-11-15 08:31:30] message here
// $t.format = "\\[?year(\\d+)-?month(\\d+)-?day(\\d+) ?hour(\\d+):?minute(\\d+):?second(\\d+)\\] (.*)";

// 06/22/2012-09:17:52.69610060 [14B0] 06/22/2012-09:17:52.69610060 [14B0]    File was created: 
// $t.format = "?month(\\d+)/?day(\\d+)/?year(\\d+)-?hour(\\d+):?minute(\\d+):?second(\\d+).?nanosecond(\\x+) \\[?tid(\\x+)\\]\\s*(.*)";

// MSI (s) (50:38) [18:29:32:497]: PROPERTY CHANGE: Adding StartPage_VideoThumbnailImages100DPI.700D050E_20D6_4BF6_A413_3BC5783D84FC property. Its value is 'C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\StartPages\StartPageVideoThumbnails\100DPI'.
$t.format = "|time||tid|||user1|";

$v.setViewLayout(30, 0.3);
$v.setColumns(["line", "tid", "time", "user1", "message"]);

// Csi trace specific bindings
// ctrl+i - display csi only 
// ctrl+t - display thread only
// ctrl+h - 
var csiColl = null;
var limitCsi = false;

var threadColl = null;
var threadId = null;
var limitThread = false;
var threadCollIdx = 1;

// limits trace scope to current thread
function toggleThreadScope()
{
    limitThread = !limitThread;

    if(limitThread)
    {
        var line = new TraceLine($.view.currentLine);
        if(threadId != line.thread)
        {
            var threadQuery = $.trace.where({ "tid" : line.thread });
            threadColl = threadQuery.asCollection();
            threadId = line.thread;
        }
        viewSources[threadCollIdx] = threadColl;
    }
    else
    {
        viewSources[threadCollIdx] = null;
    }

    updateViewSource();
}

// limits trace scope to csi only
function toggleCsiScope()
{
    limitCsi = !limitCsi;
    updateViewSource();
}


// exclude lines matching condition from the scope
// TODO. has to be aggregated with csi and trace
var scopeColl = null;
function exlcudeLines(condition) {
    var exc = $.trace.where(condition).asCollection();
    if (scopeColl == null) {
        scopeColl = scopeColl.combine(exc.inverse());
    }
    else {
        scopeColl = scopeColl.combine(exc.inverse());
    }

    $.trace.setScope(scopeColl);
}

$.shortcuts.add("ctrl+t", toggleThreadScope);
$.shortcuts.add("ctrl+i", toggleCsiScope);
