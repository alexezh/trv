// shortcuts for parts of $ object
var $t = $.trace;
var $v = $.view;
var commandHelp = [];
function addCommandHelp(cmd, descStr) {
    commandHelp.push({ id : cmd, desc : descStr });
}

function $p(p) { $.print(p); }
addCommandHelp("p(text)", "print <text> to output window");

// stores last filter created with vf call
var $f = null;

function asInt(val) {
    if (typeof val == 'number') {
        return val;
    }
    else if (typeof val == 'string') {
        return parseInt(val);
    }
    else {
        throw "invalid type";
    }
}

// add filter provided collection, color and title 
function af(coll, color, title) {
    $f = $.tagger.addFilter(coll, color, title);
    return $f;
}

// add filter by running query against trace
function a(condition, color, title) {
    af($.trace.where(condition), color, title);
}
$.dotexpressions.add("a", a);
addCommandHelp("a(condition, color)", "highlight lines which match <condition> with <color>");

// enable / disable filter by id
/*
function df(id) { $.tagger.enableFilter(asInt(id), false); }
$.dotexpressions.add("df", df);
addCommandHelp("df(id)", "disable filter with <id>");

function ef(id) { $.tagger.enableFilter(asInt(id), true); }
$.dotexpressions.add("ef", ef);
addCommandHelp("ef(id)", "enable filter with <id>");
*/

// remove filter by id
function rf(id) { $.tagger.removeFilter(asInt(id)); }
$.dotexpressions.add("rf", rf);
addCommandHelp("rf(id)", "remove filter with <id>");

// view.setSource sets a collection to be displayed in trace window
// the collection is generated as intersection of collections from viewSources array
// viewSources[0] contains collection produces by tagger. Other elements of array are free to use
var viewSources = [];

// intersects all line
function updateViewSource() {
    var viewSource = null;

    for (var i = 0; i < viewSources.length; i++)
    {
        if (viewSources[i] != null)
            viewSource = (viewSource == null) ? viewSources[i] : viewSource.intersect(viewSources[i]);
    }

    $.view.setSource(viewSource);
}

// show/hide unselected text
var limitTagged = false;
var taggedColl = null;
var taggedCollIdx = 0;
function sf() 
{ 
    limitTagged = !limitTagged;
    if(limitTagged)
    {
        if (taggedColl == null)
            taggedColl = $.tagger.asCollection();

        viewSources[taggedCollIdx] = taggedColl;
    }
    else
    {
        viewSources[taggedCollIdx] = null;
    }

    updateViewSource();
}

$.tagger.onChanged(function ()
{
    if(limitTagged)
    {
        taggedColl = $.tagger.asCollection();
        viewSources[taggedCollIdx] = taggedColl;
        updateViewSource();
    }
});

$.dotexpressions.add("sf", sf);
addCommandHelp("sf()", "switch between displaying all lines and lines which match filters");
$.shortcuts.add("ctrl+h", sf);

// print filters
function pf() { $.tagger.printFilters(); }
$.dotexpressions.add("pf", pf);
addCommandHelp("pf()", "print current filters");

// move cursor to console and add .a in beginning
function startEditFilter()
{
    $.console.setFocus();
    $.console.setText(".a ");
}
$.shortcuts.add("ctrl+a", startEditFilter);

// find support
var findColl = null;
var findCollIdx = 0;
function find(condition) {
    findColl = $.trace.where(condition).asCollection();
    if (findColl.count > 0) {
        findCollIdx = 0; 
        $.view.setFocusLine(findColl.getLine(findCollIdx).index);
        findCollIdx++;
    }
    else {
        findColl = null;
    }
}
function findNext(condition) {
    if (findCollIdx < findColl.count) {
        $.view.setFocusLine(findColl.getLine(findCollIdx).index);
        findCollIdx++;
    }
}
function startEditFind() {
    $.console.setFocus();
    $.console.setText(".f ");
}
$.dotexpressions.add("f", find);
$.dotexpressions.add("n", findNext);
$.shortcuts.add("ctrl+f", startEditFind);
$.shortcuts.add("ctrl+f3", findNext);
addCommandHelp("f()", "fine first line which matches <condition>");
addCommandHelp("n()", "fine next line which matches <condition>");

// history
var $h = $.history;
function hp() { $.history.print(); }
function he(id) { $.history.exec(id); }
addCommandHelp("hp()", "print command history");
addCommandHelp("he(id)", "execute command <id> from history");

// load/reload scripts
var lastScript = null;
function load(file) {
    lastScript = file;
    $.import(file);
}

function reload() {
    if (lastScript != null)
        $.import(lastScript);
}
addCommandHelp("load(file)", "load script from <file>");
addCommandHelp("reload()", "reload last script");

$.dotexpressions.add("r", reload);

var Black = "Black";
var DarkBlue = "DarkBlue";
var DarkGreen = "DarkGreen";
var DarkCyan = "DarkCyan";
var DarkRed = "DarkRed";
var DarkMagenta = "DarkMagenta";
var Brown = "Brown";
var LightGray = "LightGray";
var DarkGray = "DarkGray";
var Blue = "Blue";
var Green = "Green";
var Cyan = "Cyan";
var Red = "Red";
var Magenta = "Magenta";
var Yellow = "Yellow";
var White = "White";

$.import("trv.test.js");
$.import("trv.help.js");
