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
var threadId = null;
var limitThread = false;
var threadColl = null;
var threadSource = {
    name: "thread",
    key: "ctrl+t",
    update: function () {
        $.print("update thread. filtered=" + limitThread);
        if (limitThread) {
            return threadColl;
        }
        else {
            return null;
        }
    },
    enabled: function () {
        return limitThread;
    }
};

viewSources.push(threadSource);

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
        showFiltered(true);
    }
    else
    {
        threadColl = null;
        updateViewSource();
    }
}

$.shortcuts.add("ctrl+t", toggleThreadScope);

// Filter by sub system. A user can add to subsystem 

var categories = [];
var limitCategory = false;
var categorySourceDirty = true;
var categoryColl = null;
var categorySource = {
    name: "category",
    key: "ctrl+1",
    update: function () {
        $.print("update category. filtered=" + limitCategory);
        if (limitCategory) {
            if (categoryColl != null)
                return categoryColl;

            for (var it in categories) {
                categoryColl = (categoryColl == null) ? categories[it].coll : categoryColl.combine(categories[it].coll);
            }

            return categoryColl;
        }
        else {
            return null;
        }
    },
    enabled: function () {
        return limitCategory;
    }
};

viewSources.push(categorySource);

function addCategory(sub)
{
    for (var it in categories) {
        if (categories[it].name === sub) {
            $.print("found category " + sub);
            return;
        }
    }

    $.print("add category " + sub);
    var query = $.trace.where({ "user1": sub });
    categories.push({ name: sub, coll: query.asCollection() });

    limitCategory = true;
    categoryColl = null;
    updateViewSource();
}

function removeCategory(sub) {
    for (var it in categories) {
        if (categories[it].name === sub) {
            $.print("remove category " + sub);
            categories.splice(it, 1);
            break;
        }
    }

    limitCategory = true;
    categoryColl = null;
    updateViewSource();
}

function toggleCategoryFilter() {
    limitCategory = !limitCategory;

    if (limitCategory) {
        showFiltered(true);
    }
    else {
        updateViewSource();
    }
}

$.shortcuts.add("ctrl+e", function () {
    var line = new TraceLine($.view.currentLine);
    addCategory(line.user1);
    
    var names = "";
    for (var it in categories) {
        names += " \"" + categories[it].name + "\"";
    }

    $.print("Selected categories " + names);
});

$.shortcuts.add("alt+1", toggleCategoryFilter);

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

