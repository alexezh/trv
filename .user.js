var qc = new TraceCollection();
var qf = $.view.filter(qc, Blue, "quick collection");

// add current line to quick collection
function ql() { qc.addline($.view.currentline); }
$.dotexpressions.add("ql", ql);
