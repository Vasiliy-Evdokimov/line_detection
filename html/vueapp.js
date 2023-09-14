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
        draw_interval: null
    },
    methods: {
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
                    this.status_text += ' успешно!';
                }
            })
            .catch((err) => {
                if (aApplyStatus) {
                    this.status_ok = false;
                    this.status_text += ' возникла ошибка!';
                }                    
            });
        },        
        get_params: function() {
            this.server_request(
                "Получение текущих параметров...",
                "/get_params",
                "GET",
                null,
                this.get_params_callback
            );
        },
        get_params_callback: function(data) {
            this.cur_params = JSON.parse(JSON.stringify(data));
            if (!this.params_init) {
                this.params_init = true;
            }
            this.new_params = JSON.parse(JSON.stringify(this.cur_params));            
        },
        apply_params: function() {    
            let cnv = this.canvas;
            let ctx = this.context;

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
            this.get_params();            
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
            var res = params_descriptions.get(param_name);
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
            this.get_points();
        }

    },
    mounted: function () {
        this.get_params();
        //
        this.draw_canvas = document.getElementById("graph");
        this.draw_context =  this.draw_canvas.getContext("2d");
        //
        this.draw_interval = setInterval(this.draw, 500);
    }
});

// type: 1 - int; 2 - string; 3 - bool; 4 - ip
var params_descriptions = new Map([
    ['CAM_ADDR_1',  { type: 2, descr: 'RTSP адрес 1й камеры'}],
    ['CAM_ADDR_2',  { type: 2, descr: 'RTSP адрес 2й камеры'}],
    ['UDP_ADDR',    { type: 4, descr: 'UDP адрес для обмена данными'}],
    ['UDP_PORT',    { type: 1, descr: 'UDP порт для обмена данными'}],
    //
    ['NUM_ROI',     { type: 1, descr: 'Количество горизонтальных полос'}],
	['NUM_ROI_H',   { type: 1, descr: 'Количество горизонтальных полос, которые делим на вертикальные'}],
	['NUM_ROI_V',   { type: 1, descr: 'Количество вертикальных полос'}],
	//	
	['SHOW_GRAY',       { type: 3, descr: 'Показывать изображение после преобразований'}],
	['DRAW_DETAILED',   { type: 3, descr: 'Показывать детали анализа изображения'}],
	['DRAW_GRID',       { type: 3, descr: 'Показывать сетку разбиения изображения'}],
	['DRAW',            { type: 3, descr: 'Показывать изображение'}],
	//
	['MIN_CONT_LEN',    { type: 1, descr: 'Минимальная длина контура при поиске фрагмента линии'}],
	['HOR_COLLAPSE',    { type: 1, descr: 'Усреднение горизонтальных линий, если расстояние между ними меньше чем'}],
	//
	['GAUSSIAN_BLUR_KERNEL',    { type: 1, descr: 'Ядро преобразования для GaussianBlur - (NxN)'}],
	['MORPH_OPEN_KERNEL',       { type: 1, descr: 'Ядро преобразования для MorphologyEx(MORPH_OPEN) - (NxN)'}],
	['MORPH_CLOSE_KERNEL',      { type: 1, descr: 'Ядро преобразования для MorphologyEx(MORPH_CLOSE) - (NxN)'}],
	//
	['THRESHOLD_THRESH', { type: 1, descr: 'Параметр (thresh) функции Threshold'}],
	['THRESHOLD_MAXVAL', { type: 1, descr: 'Параметр (maxval) функции Threshold'}]
]);