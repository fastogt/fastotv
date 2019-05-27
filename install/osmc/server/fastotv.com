upstream node_domain {
    server 127.0.0.1:3000;
}

upstream node_frontend {
    server 127.0.0.1:8080;
}

server {
    listen 80 default_server;

#    root /home/binhot/public_html/front_end;
#    index index.ejs;

    server_name www.siteonyourdevice.com siteonyourdevice.com;

#    location / {
#        try_files $uri $uri/ =404;
#    }


    location / {
        proxy_pass http://node_frontend;
        #try_files $uri $uri/ =404;
    }

#    location = /node/server_details.html {
#        proxy_pass http://node_domain;
#       proxy_http_version 1.1;
#       proxy_set_header Upgrade $http_upgrade;
#       proxy_set_header Connection "upgrade";
#       proxy_set_header Host $host;
#       proxy_redirect     off;
#       proxy_set_header   X-Real-IP        $remote_addr;
#       proxy_set_header   X-Forwarded-For  $proxy_add_x_forwarded_for;
#    }

#    error_page 404 /404.html;
#    error_page 500 502 503 504 /50x.html;
#    location = /50x.html {
#        root /usr/share/nginx/html;
#    }
}

upstream proxy_backend {
    server 127.0.0.1:8040;
}

server {
    listen 80;

    server_name www.proxy.siteonyourdevice.com proxy.siteonyourdevice.com;

    #root /home/binhot/public_html;
    index index.html index.htm;

    client_max_body_size 10G;

    location / {
        proxy_pass http://proxy_backend;
        proxy_ignore_client_abort on;
        proxy_set_header Host $host;

        #proxy_redirect     off;
        #proxy_set_header   X-Real-IP        $remote_addr;
        #proxy_set_header   X-Forwarded-For  $proxy_add_x_forwarded_for;
    }
}
