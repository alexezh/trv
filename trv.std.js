// shortcuts for parts of $ object
var $t = $.trace;
var $v = $.view;
function $p(p) { $.print(p); }

// stores last filter created with vf call
var $f = null;

// add filter 
function af(coll, color, title) 
{
    $f = $.tagger.addFilter(coll, color, title); 
}
function vfi(coll, color, title) { $.tagger.filterinteractive(coll, color, title); }

function rf(id) { $.tagger.removeFilter(id); }
$.dotexpressions.add("rf", rf);

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
        viewColls[taggedCollIdx] = taggedColl;
        updateViewSource();
    }
});

$.dotexpressions.add("sf", sf);
$.shortcuts.add("ctrl+h", sf);

// print filters
function pf() { $.tagger.printFilters(); }
$.dotexpressions.add("pf", pf);


function a(condition, color, title) 
{ 
    af($.trace.where(condition), color, title); 
}
$.dotexpressions.add("a", a);

function df(id) { $.tagger.enableFilter(id, false); }
$.dotexpressions.add("df", df);

function ef(id) { $.tagger.enableFilter(id, true); }
$.dotexpressions.add("ef", ef);

// move cursor to console and add .a in beginning
function startEditFilter()
{
    $.console.setFocus();
    $.console.setText(".a ");
}
$.shortcuts.add("ctrl+a", startEditFilter);


// history
var $h = $.history;
function hp() { $.history.print(); }
function he(id) { $.history.exec(id); }

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
