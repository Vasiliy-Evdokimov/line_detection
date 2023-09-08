const sever_url = "http://127.0.0.1:8000";

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
        server_request(aStatusText, aURL, aMethod, aDataObj, aCallback) {
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
                console.log(JSON.stringify(data));
                if (data.message == "error") throw "error";
                aCallback(data);
            })
            .then(() => {
                this.status_ok = true;
                this.status_text += ' успешно!';
            })
            .catch((err) => {
                this.status_ok = false;
                this.status_text += ' возникла ошибка!';                
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
                this.get_points_callback
            );
        },
        get_points_callback: function(data) {            
            //
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
        draw: function() {
            let ctx = this.draw_context;
            //
            ctx.fillStyle = "rgb(200,0,0)";
            ctx.fillRect(10, 10, 55, 50);
            ctx.fillStyle = "rgba(0, 0, 200, 0.5)";
            ctx.fillRect(30, 30, 55, 50);
        }

    },
    mounted: function () {
        this.get_params();
        //
        this.draw_canvas = document.getElementById("graph");
        this.draw_context =  this.draw_canvas.getContext("2d");
        //        
        this.draw_interval = setInterval(this.draw, 50);
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