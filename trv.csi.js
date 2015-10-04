// This module contains CSI specific functions and can be used as an example for other content specific parsing

// Set pare format. Values are tab separated. Map column 2 to time, 4 to thread id, 7 to user1
$t.setFormat("time||tid||user1|||", "\t");

// Show columns which we care about
$v.setColumns(["line", "tid", "time", "user1", "message"]);

// Set height of command window (30 in pixels) and minimal percentage used by output
$v.setViewLayout(30, 0.4);

// Filter by thread id
var threadColl = null;
var threadId = null;
var limitThread = false;
var threadCollIdx = 1;

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

$.shortcuts.add("ctrl+t", toggleThreadScope);

// Filter by sub system

var subColl = null;
var subSystem = [ [ "Csi Cache", "Csi Cache Recovery" ] ];
var limitSub = false;
var subCollIdx = 2;

function toggleSubsystemScope() {
    limitSub = !limitSub;

    if (limitSub) {
        for(var sub in subSystem) {
            var query = $.trace.where({ "user1": subSystem[sub] });
            subColl = (subColl == null) ? query.asCollection() : subColl.combine(query.asCollection());
        }

        viewSources[subCollIdx] = subColl;
    }
    else {
        viewSources[subCollIdx] = null;
    }

    updateViewSource();
}

$.shortcuts.add("ctrl+u" , toggleSubsystemScope);

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

function renderLine(idx, tid, time, sub, message) {
    this.idx = idx;
}
function setupRender()
{
    // var timeExp = /\s+\S([:\.\d]+)/;
    $.view.onRender(function (traceLine) {
        // var resTime = timeExp.exec(line.time);
        // simply copy data from input line
        // field name match column names
        return { tid: traceLine.tid, user1: traceLine.user1, msg: traceLine.msg };
    });
}

//setupRender();