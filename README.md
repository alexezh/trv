# README #

# Overview
Trv.js is a trace view program which utilizes JavaScript to enable powerful filtering options. 

Imagine the following case. You have a server application processing a lot of requests. The request processing takes a lot of steps all of each is done asynchronously; multiple requests can be processed at the same time. You want to see traces which belongs to a single request. With a simple tracing program you load trace into a viewer, and by using traceid or some other mechanism to find all traces for a request.

With trv.js you can minimize the amount of repetitive work by writing following script to perform the processing.

    function select_op()	
    {
        // find all traces for operation start
        var starts = $q.where("op started");
        // find all traces for operation end
        var stops = $q.where("op ended");
        var opexp = /op started (\d+)/i;

        // match op start to op end
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
 
        // for each pair, get all trace lines in between
        var ranges = pairs.map(function(pair) { return $t.fromrange(pair[0], pair[1]); }); 
        // select the first request and return an iterator
        return $v.selectinteractive(ranges, Red, "Request");
    }

Now in the console you can run

    cur = select_op()

To select the first request. Now you can run

    cur.next()

To display trace lines for the next request.

# Framework reference
Trv.js exposes a set of APIs to allow script code to access trace info, perform filtering and so on. The documentation is located [here](Framework reference). 

# Building
To build trv.js you need VS2013. You also have to build V8 engine and set V8 environment variable to point to your copy of V8.