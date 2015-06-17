# VCL examples.

# buffer_req_body(BYTES size)
# It can be called only from vcl_recv.
sub vcl_recv {
	if(bodyaccess.buffer_req_body(110B)) {
		...
	}
}

/------------------------------------------------------------------------------/

# len_req_body()
# It can be called only from vcl_recv.
sub vcl_recv {
	if(bodyaccess.buffer_req_body(110B)) {
		set req.http.x-len = bodyaccess.len_req_body();
	}
}

sub vcl_deliver {
	set resp.http.x-len = req.http.x-len;
}

/------------------------------------------------------------------------------/

# hash_req_body()
# It can be called only form vcl_hash.
sub vcl_recv {
	if(bodyaccess.buffer_req_body(110B)) {
		return (hash);
	}
}

sub vcl_hash {
	bodyaccess.hash_req_body();
	return (lookup);
}

/------------------------------------------------------------------------------/

# rematch_req_body(STRING re)
# It can be called only form vcl_recv.
sub vcl_recv {
	if (bodyaccess.buffer_req_body(10KB)) {
		set req.http.x-re = bodyaccess.rematch_req_body("Regex");
	}
}

sub vcl_deliver {
	set resp.http.x-re = req.http.x-re;
}

/------------------------------------------------------------------------------/

# forcing Varnish to use the same request method also for backend request.
# Note without this part of VCL every request method will be converted
# to GET on backend side.
sub vcl_recv {
        if (req.method == "POST" || req.method == "PUT") {
                set req.http.x-method = req.method;
	}
}

sub vcl_backend_fetch {
        set bereq.method = bereq.http.x-method;
}

/------------------------------------------------------------------------------/

# skeleton VCL using all the functions from this Vmod.
sub vcl_recv {
        if (req.method == "POST" || req.method == "PUT") {
                set req.http.x-method = req.method;
                if (bodyaccess.buffer_req_body(110KB)) {
                        set req.http.x-len = bodyaccess.len_req_body();
                        set req.http.x-re = bodyaccess.rematch_req_body("Regex"$
                }
        }
        return(hash);

}

sub vcl_hash {
        bodyaccess.hash_req_body();
        return (lookup);
}

sub vcl_backend_fetch {
        set bereq.method = bereq.http.x-method;
}

sub vcl_deliver {
        set resp.http.x-len = req.http.x-len;
        set resp.http.x-re = req.http.x-re;
}
 
