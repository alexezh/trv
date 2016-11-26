// This module contains CSI specific functions and can be used as an example for other content specific parsing

$.onLoaded(function () {
    // Set pare format. Values are tab separated. Map column 2 to time, 4 to thread id, 7 to user1
    $t.setFormat("time||tid||user1|user2||", "\t");

    // Show columns which we care about
    $v.setColumns(["line", "tid", "time", "user1", "user2", "message"]);

    // Set height of command window (30 in pixels) and minimal percentage used by output
    $v.setViewLayout(30, 0.4);

    setupRender();
});

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

// Filter by sub system. A user can add to subsystem 

var categories = [];
var limitCategory = false;
var categoryCollIdx = 2;
var categoryColl = null;

function toggleCategory(sub) {
    var add = true;
    for (var it in categories) {
        if (categories[it].name == sub) {
            categories.splice(it, 1);
            add = false;
            break;
        }
    }

    if (add) {
        var query = $.trace.where({ "user1": sub });
        categories.push({ name: sub, coll: query.asCollection() });
    }

    categoryColl = null;
    for (var it in categories) {
        categoryColl = (categoryColl == null) ? categories[it].coll : categoryColl.combine(categories[it].coll);
    }

    viewSources[categoryCollIdx] = categoryColl;
}

function toggleCategoryFilter() {
    limitCategory = !limitCategory;

    if (limitCategory) {
        viewSources[categoryCollIdx] = categoryColl;
    }
    else {
        viewSources[categoryCollIdx] = null;
    }

    updateViewSource();
}

$.shortcuts.add("ctrl+e", function () {
    var line = new TraceLine($.view.currentLine);
    toggleCategory(line.user1);
    
    var names = "";
    for (var it in categories) {
        names += " \"" + categories[it].name + "\"";
    }

    if (!limitCategory) {
        toggleCategoryFilter();
    }

    $.print("Selected categories " + names);
});

$.shortcuts.add("alt+e", toggleCategoryFilter);

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
    var timeExp = /\s+\S([:\.\d]+)/;
    $.view.onRender(function (traceLine) {
        var res = timeExp.exec(traceLine.time);
        return {
            time: ((res != null) ? res[0] : traceLine.time),
            user1: traceLine.user1,
            user2: traceLine.user2,
            msg: traceLine.msg
        };
    });
}

