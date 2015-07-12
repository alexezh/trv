var cur;

function test_or_string()
{
	$v.filter($q.where("Run").or("inside"), Red);
}

function test_and_string()
{
	$v.filter($q.where("Run").and("side"), Red);
}

function test_quick_collection()
{
 	var c = [];
	c.push($v.currentline);
	$.collections.quick.add($v.currentline);
}

function test1()
{
	var qrange = $q.where("op started").or("op ended");
	var opranges = qrange.pair().map(function(pair) { return $t.fromrange(pair[0], pair[1]); }); 

	$v.filter(opranges, Red);
}

function test2()
{
	var qrange = $q.where("op started").or("op ended");
	var opranges = qrange.pair().map(function(pair) { return $t.fromrange(pair[0], pair[1]); }); 

	cur = $v.selectinteractive(opranges, "Hello", Red);
}

function test3()
{
	var qrange = $q.where("op started").or("op ended");
	var opranges = qrange.pair().map(function(pair) { return $t.fromrange(pair[0], pair[1]); }); 

	cur = $v.selectinteractive(opranges, Red);
}

function test4()
{
	var starts = $q.where("op started");
	var stops = $q.where("op ended");
	var opexp = /op started (\d+)/i;

	var pairs = starts.map(function(start) 
	{
		var res = opexp.exec(start.msg);
		if(res == null)
		{
			$.print(start.msg);
			throw "incorrect syntax for trace line";
		}
		return [start, stops.find(res[1])]; 
	}); 

	var ranges = pairs.map(function(pair) { return $t.fromrange(pair[0], pair[1]); }); 

	cur = $v.selectinteractive(ranges, Red, "Hello");
}

function test10()
{
//	var qend = $q.where("started");
	$t.line(1).print();
//	var d = new Date(79, 11);
//	$p(d);
//	var qtime = $q.where(function(x) { return x.time < 08:32:07 });
	var qtid = $q.where(function(x) { return x.thread == 1 });
	$v.select(qtid, Red);
}
