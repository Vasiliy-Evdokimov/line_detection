const url = "http://127.0.0.1:8000";

var app = new Vue({
    el: '#app',
    data: {
        params_init: false,
        cur_params: {},
        new_params: {},
        status_ok: true,
        status_text: ""
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
        get_params: function() {

            this.status_text = 'Получение текущих параметров...';

            fetch(
                url + "/get_params",
                {
                    method: "GET",
                    headers: { "Content-Type": "application/json" }
                }
            )
            .then((response) => {
                response.json().then(
                    (data) => {
                        this.cur_params = JSON.parse(JSON.stringify(data));
                        if (!this.params_init) {
                            this.params_init = true;
                        }
                        Object.assign(this.new_params, this.cur_params);
                    });
                this.status_ok = true;
                this.status_text += ' успешно!';
            })
            .catch((err) => {
                this.status_ok = false;
                this.status_text += ' возникла ошибка!\n' + err;
                console.error(err);
            });

        },
        apply_params: function() {

            this.status_text = 'Применение новых параметров...';
            
            let buf = this.process_params_data(this.new_params);

            fetch(
                url + "/apply_params",
                {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify(buf)
                }
            )
            .then((response) => {
                response.json().then(
                    (data) => {
                        console.log(JSON.stringify(data));
                    });
                this.status_ok = true;
                this.status_text += ' успешно!';
            })
            .catch((err) => {
                this.status_ok = false;
                this.status_text += ' возникла ошибка!\n' + err;
                console.error(err);
            });

        },
        save_params: function() {
            alert('save_params()');
        },
        get_param_info: function(param_name) {
            param_name = param_name.slice(3);
            var res = params_descriptions.get(param_name);
            if (!res) res = param_name;
            return res;
        },
        is_checkbox_param(name) {
            return checkboxes_params.has(name);
        }
    },
    mounted: function () {
        this.get_params();
    }
});

var params_descriptions = new Map([

    ['CAM_ADDR_1', 'RTSP адрес 1й камеры'],
    ['CAM_ADDR_2', 'RTSP адрес 2й камеры'],
    ['UDP_ADDR', 'UDP адрес для обмена данными'],
    ['UDP_PORT', 'UDP порт для обмена данными'],

    ['NUM_ROI', 'Количество горизонтальных полос'],
	['NUM_ROI_H', 'Количество горизонтальных полос, которые делим на вертикальные'],
	['NUM_ROI_V', 'Количество вертикальных полос'],
	//	
	['SHOW_GRAY', 'Показывать изображение после преобразований'],
	['DETAILED', 'Показывать детали анализа изображения'],
	['DRAW_GRID', 'Рисовать сетку разбиения изображения'],
	['DRAW', 'Показывать изображение. Предыдущие 3 опции действуют только при включенной этой'],
	//
	['MIN_CONT_LEN', 'Минимальная длина контура при поиске фрагмента линии'],
	['HOR_COLLAPSE', 'Усреднение горизонтальных линий, если расстояние между ними меньше чем'],
	//
	['GAUSSIAN_BLUR_KERNEL', 'Ядро преобразования для GaussianBlur - (NxN)'],
	['MORPH_OPEN_KERNEL', 'Ядро преобразования для MorphologyEx(MORPH_OPEN) - (NxN)'],
	['MORPH_CLOSE_KERNEL', 'Ядро преобразования для MorphologyEx(MORPH_CLOSE) - (NxN)'],
	//
	['THRESHOLD_THRESH', 'Параметр (thresh) функции Threshold'],
	['THRESHOLD_MAXVAL', 'Параметр (maxval) функции Threshold']

]);

var checkboxes_params = new Set([    
    "SHOW_GRAY",
    "DETAILED",
    "DRAW_GRID",
    "DRAW"
]);