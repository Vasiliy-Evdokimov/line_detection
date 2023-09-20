const sever_url = location.origin;

var app = new Vue({
    el: '#app',
    data: {
        params_init: false,
        cur_params: {},
        new_params: {},
        status_ok: true,
        status_text: "",
        //
        draw_canvas: null,
        draw_context: null,
        draw_interval: null,
        //
        web_show_lines: false,
        web_interval: 50,
        //
        params_descriptions: new Map()
    },
    methods: {
        get_param_by_name: function(aParams, aName) {
            for (key in aParams)
                if (key.slice(3) == aName)
                    return aParams[key];                
            return null;
        },
        process_params_data: function(data) {
            let res = {};
            let new_key = '';
            for (key in data) {
                new_key = key.slice(3);
                res[new_key] = data[key];
            }
            return res;
        },
        server_request(aStatusText, aURL, aMethod, aDataObj, aCallback, aApplyStatus = true, aLogResponse = true) {
            if (aApplyStatus)
                this.status_text = aStatusText;
            //
            var request = new Request(sever_url + aURL);
            var init = {
                method: aMethod,
                headers: { "Content-Type": "application/json" }
            };
            if (aMethod == "POST")
                init.body = JSON.stringify(aDataObj);

            fetch(request, init)
            .then((response) => response.json())
            .then((data) => {
                if (aLogResponse)
                    console.log(JSON.stringify(data));
                if (data.message == "error") throw "error";
                aCallback(data);
            })
            .then(() => {
                if (aApplyStatus) {
                    this.status_ok = true;                    
                }
            })
            .catch((err) => {
                if (aApplyStatus) {
                    this.status_ok = false;                    
                }                    
            });
        },        
        get_params: function(aApplyStatus = true) {
            this.server_request(
                "Получение текущих параметров...",
                "/get_params",
                "GET",
                null,
                this.get_params_callback,
                aApplyStatus
            );           
        },
        get_params_callback: function(data) {
            this.cur_params = JSON.parse(JSON.stringify(data));
            if (!this.params_init) {
                this.params_init = true;
            }
            this.new_params = JSON.parse(JSON.stringify(this.cur_params));            
            this.check_params();
        },
        get_config_map: function() {
            this.server_request(
                "Получение описаний параметров...",
                "/get_config_map",
                "GET",
                null,
                this.get_config_map_callback
            );           
        },
        get_config_map_callback: function(data) {
            let params = JSON.parse(JSON.stringify(data));
            this.params_descriptions.clear();
            for (item of params.result)
                this.params_descriptions.set(item.name,
                    { "type": item.type, "descr": item.descr });
        },
        check_params: function() {
            this.web_show_lines = (this.get_param_by_name(this.cur_params, "WEB_SHOW_LINES") > 0);
            this.web_interval = parseInt(this.get_param_by_name(this.cur_params, "WEB_INTERVAL"));
            //
            if (this.draw_interval) 
                clearInterval(this.draw_interval);
            this.draw_interval = setInterval(this.draw, this.web_interval);
        },
        apply_params: function() {    
            let cnv = this.canvas;
            let ctx = this.context;
            //
            if (!(this.check_form(this.new_params)))
                return;
            //
            this.server_request(
                "Применение новых параметров...",
                "/apply_params",
                "POST",
                this.process_params_data(this.new_params),
                this.apply_params_callback
            );
        },
        apply_params_callback: function(data) {
            this.cur_params = JSON.parse(JSON.stringify(this.new_params));
            this.check_params();
        },
        save_params: function() {            
            this.server_request(
                "Сохранение новых параметров...",
                "/save_params",
                "GET",
                null,
                this.save_params_callback
            );
        },
        save_params_callback: function(data) {            
            this.get_params(false);            
        },
        get_points: function() {            
            this.server_request(
                "Получение результатов распознавания...",
                "/get_points",
                "GET",
                null,
                this.get_points_callback,
                false,
                false
            );
        },
        get_points_callback: function(data) {
            let ctx = this.draw_context;
            //
            let w0 = data.result[0].width;
            let h0 = data.result[0].height;
            let w1 = data.result[1].width;            
            let h1 = data.result[1].height;
            //
            let canvas_width = w0 + w1;
            let canvas_height = Math.max(h0, h1);
            if ((this.draw_canvas.width != canvas_width) ||
                (this.draw_canvas.height != canvas_height)) 
            {
                this.draw_canvas.width = canvas_width;
                this.draw_canvas.height = canvas_height;
            }
            //
            let offset = 0;
            for (let i = 0; i < 2; i++) {                

                let res = data.result[i];
                let width = res.width;
                let height = res.height;
                //            
                ctx.beginPath();
                ctx.rect(offset, 0, width, height);
                ctx.closePath();
                ctx.strokeStyle = "yellow";
                ctx.fillStyle = "black";
                ctx.fill();
                ctx.stroke();
                ctx.strokeStyle = "black";
                //
                if (res.fl_error) {
                    ctx.beginPath();                                
                    ctx.fillStyle = "red";
                    ctx.arc(50 + offset, 50, 20, 0, this.get_radians(360));
                    ctx.stroke();
                    ctx.fill();
                } else {
                    ctx.lineWidth = 3;
                    ctx.strokeStyle = "green";
                    let res_point;
                    ctx.beginPath();
                    ctx.moveTo((width / 2) + offset, height);
                    for (res_point of res.res_points)
                        ctx.lineTo(res_point.x + offset, res_point.y);            
                    ctx.stroke();
                    //
                    ctx.strokeStyle = "red";
                    let hor_y;
                    for (hor_y of res.hor_ys)
                    {
                        ctx.beginPath();
                        ctx.moveTo(offset, hor_y);
                        ctx.lineTo(width + offset, hor_y);
                        ctx.stroke();
                    }
                }

                offset = width;

            }            
        },
        get_param_info: function(param_name) {
            param_name = param_name.slice(3);
            var res = this.params_descriptions.get(param_name);
            return res;
        },
        is_checkbox_param: function(name) {
            return checkboxes_params.has(name);
        },
        is_numeric: function(str) {
            return !isNaN(str) &&
                   !isNaN(parseFloat(str));
        },
        is_ip_address: function (str) {
            return (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(str));
        },
        check_form: function(params_obj) {
            let err_params = "";
            //
            for (key in params_obj) {
                let value = params_obj[key];
                if (!value) continue;
                let info = this.get_param_info(key);
                if (!info) continue;
                //
                let err_fl = false;
                switch(info.type) {
                    case(1):    // int
                        {
                            err_fl = !(this.is_numeric(value));                                 
                            break;
                        }
                    case(2):    // string
                        {                        
                            break;
                        }
                    case(3):    // bool
                        {
                            err_fl = (!(this.is_numeric(value)) || (value > 2 ));
                            break;
                        }
                    case(4):    //  ip
                        {
                            err_fl = !(this.is_ip_address(value));   
                            break;
                        }                    
                }
                if (err_fl)
                    err_params += info.descr + "; ";
            }
            //
            if (err_params != "") {
                this.status_text = "Некорректые значения следующих параметров: " + err_params;
                this.status_ok = false;                
                return false;
            }
            //
            return true;
        },
        get_radians: function(degrees) {
            return (Math.PI / 180) * degrees;
        },
        draw: function() {
            if (this.web_show_lines)
                this.get_points();
        }
    },
    mounted: function () {
        this.get_config_map();
        this.get_params();
        //
        this.draw_canvas = document.getElementById("graph");
        this.draw_context =  this.draw_canvas.getContext("2d");      
    }
});