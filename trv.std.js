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
    if (typeof val === 'number') {
        return val;
    }
    else if (typeof val === 'string') {
        return parseInt(val);
    }
    else {
        throw "invalid type";
    }
}

var TaggerItem = function (set, color, desc) {
    if (set instanceof Query) {
        this.Collection = set.asCollection();
    } else if (set instanceof TraceCollection) {
        this.Collection = set;
    }
    else {
        throw "unsupported set type";
    }

    this.Id = 0;
    this.Color = color;
    this.Description = desc;
    this.Enabled = true;
    this.OnChangedHandler = null;
}

var Tagger = function () {
    this._Items = [];
}

Tagger.prototype.add = function (item) {
    var id = this._Items.length;
    item.Id = id;
    $.print("add tag #" + id);
    this._Items.push(item);
    $.tagger.addFilter(item.Collection, item.Color);
    this.invokeOnChanged();
    $.view.refresh();
    return item;
}

Tagger.prototype.getItems = function () {
    return _Items;
}

Tagger.prototype.remove = function (id) {
    for (var i in this._Items) {
        if (this._Items[i].Id == id) {
            $.print("remove tag #" + id);
            if (this._Items[i].Enabled) {
                $.tagger.removeFilter(this._Items[i].Collection);
            }
            this._Items.splice(i, 1);
            this.invokeOnChanged();
            $.view.refresh();
            break;
        }
    }
}

Tagger.prototype.enable = function (id, val) {
    for (var i in this._Items) {
        if (this._Items[i].Id == id) {
            var item = this._Items[i];
            if (item.Enabled == val)
                break;

            item.Enabled = val;
            if (val) {
                $.print("enable tag #" + id);
                $.tagger.addFilter(item.Collection, item.Color);
            } else {
                $.print("disable tag #" + id);
                $.tagger.removeFilter(item.Collection);
            }
            this.invokeOnChanged();
            $.view.refresh();
            break;
        }
    }
}

Tagger.prototype.enableIf = function (cond) {
    $.tagger.clear();
    for (i in _Items) {
        var enabled = false;
        if (typeof cond === 'object') {
            if (_Items[i] === cond) {
                enabled = true;
            }
        }

        if (enabled) {
            $.tagger.add(_Items[i]);
        }
    }
}

Tagger.prototype.onChanged = function (func) {
    this.OnChangedHandler = func;
}

Tagger.prototype.invokeOnChanged = function () {
    if (this.OnChangedHandler != null)
        this.OnChangedHandler();
}

Tagger.prototype.asCollection = function () {
    var res = null;
    for (var i in this._Items) {
        if (!this._Items[i].Enabled)
            continue;

        res = (res == null) ? this._Items[i].Collection : res.combine(this._Items[i].Collection);
    }

    return res;
}

Tagger.prototype.print = function () {
    for (var i in this._Items) {
        var item = this._Items[i];
        $.print(item.Id + " " + item.Color + " enabled:" + item.Enabled + " " + item.Description + "\r\n");
    }
}

var tagger = new Tagger();

// add filter provided collection, color and title 
function af(set, color, title) {
    $f = tagger.add(new TaggerItem(set, color, title));
    return $f;
}

// add filter by running query against trace
function a(condition, color) {
    af($.trace.where(condition), color, condition);
}
$.dotexpressions.add("a", a);
addCommandHelp("a(condition, color)", "highlight lines which match <condition> with <color>");

// enable / disable filter by id
function df(id) { tagger.enable(asInt(id), false); }
$.dotexpressions.add("df", df);
addCommandHelp("df(id)", "disable filter with <id>");

function ef(id) { tagger.enable(asInt(id), true); }
$.dotexpressions.add("ef", ef);
addCommandHelp("ef(id)", "enable filter with <id>");

// remove filter by id
function rf(id) { tagger.remove(asInt(id)); }
$.dotexpressions.add("rf", rf);
addCommandHelp("rf(id)", "remove filter with <id>");

// view.setSource sets a collection to be displayed in trace window
// the collection is generated as intersection of collections from viewSources array
// viewSources[0] contains collection produces by tagger. Other elements of array are free to use
// viewSource element is a pair of { name, coll }
var viewSources = [];
var viewSource = null;
var limitFiltered = false;
var viewSourceUpdatePending = false;

// intersects all line
function updateViewSource()
{
    if (viewSourceUpdatePending)
        return;

    viewSourceUpdatePending = false

    // delay actual update until current scripts are done
    // this way we do not have to worry about code executing many times
    $.post(function () {
        try {
            $.print("update view. filtered=" + limitFiltered);
            viewSource = null;

            if (limitFiltered) {
                for (var i = 0; i < viewSources.length; i++) {
                    var coll = viewSources[i].update();
                    if (coll != null)
                        viewSource = (viewSource == null) ? coll : viewSource.intersect(coll);
                }
            }

            $.view.setSource(viewSource);
            $.print("");
        }

        finally {
            viewSourceUpdatePending = false;
        }
    });
}

function printViewSources()
{
    for (var i = 0; i < viewSources.length; i++) {
        var enabled = viewSources[i].enabled();
        $.print(i + ": " + viewSources[i].name + " " + ((enabled) ? "enabled" : "disabled"));
    }
}

function showFiltered(val)
{
    limitFiltered = val;
    updateViewSource();
}

function toggleFiltered()
{
    limitFiltered = !limitFiltered;
    updateViewSource();
}

function sf()
{
    toggleFiltered();
}

$.dotexpressions.add("sf", toggleFiltered);
addCommandHelp("sf()", "switch between displaying all lines and lines which match filters");
$.shortcuts.add("ctrl+h", toggleFiltered);

// show/hide unselected text
var limitTagged = false;
var taggedColl = null;
var taggerSource = {
    name: "tagger",
    key: "ctrl+0",
    update: function () {
        $.print("update tagged. filtered=" + limitTagged);
        if (limitTagged) {
            if (taggedColl == null)
                taggedColl = tagger.asCollection();

            return taggedColl;
        }
        else {
            return null;
        }
    },
    enabled: function () {
        return limitTagged;
    }
};
viewSources.push(taggerSource);

function toggleTagged()
{
    limitTagged = !limitTagged;
    if (limitTagged) {
        showFiltered(true);
    }
    else {
        updateViewSource();
    }
}

$.shortcuts.add("ctrl+0", toggleTagged);

tagger.onChanged(function ()
{
    limitTagged = true;
    taggedColl = null;
    updateViewSource();
});


// print filters
function pf()
{
    tagger.print();
}
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
function find(condition)
{
    findColl = ((viewSource != null) ? viewSource : $.trace).where(condition).asCollection();
    if (findColl.count > 0) {
        findCollIdx = 0;
        $.view.setFocusLine(findColl.getLine(findCollIdx).index);
        findCollIdx++;
    }
    else {
        findColl = null;
    }
}

function findNext(condition)
{
    if (findCollIdx < findColl.count) {
        $.view.setFocusLine(findColl.getLine(findCollIdx).index);
        findCollIdx++;
    }
}

function startEditFind()
{
    $.console.setFocus();
    $.console.setText(".f ");
}
$.dotexpressions.add("f", find);
$.dotexpressions.add("n", findNext);
$.shortcuts.add("ctrl+f", startEditFind);
$.shortcuts.add("ctrl+f3", findNext);
addCommandHelp("f()", "fine first line which matches <condition>");
addCommandHelp("n()", "fine next line which matches <condition>");

$.shortcuts.add("ctrl+o", function () { $.console.setFocus()
});

// history
var $h = $.history;
function hp()
{
    $.history.print();
}
function he(id)
{
    $.history.exec(id);
}
addCommandHelp("hp()", "print command history");
addCommandHelp("he(id)", "execute command <id> from history");

// load/reload scripts
var lastScript = null;
function load(file)
{
    lastScript = file;
    $.import(file);
}

function reload()
{
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
