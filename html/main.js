"use strict";

let play_button  = document.getElementById("play");
let save_button  = document.getElementById("save");
let save_menu    = document.getElementById("save_menu");
let save_form    = document.getElementById("save_form");
let close_button = document.getElementById("close");
let design_link  = document.getElementById("link");

let user_id = localStorage.getItem("userId");
if (!user_id)
	save_button.disabled = true;

let opened = false;

function play(event)
{
	inst.exports.key_down(65);
	play_button.blur();
}

function save(event)
{
	save_menu.style.display = "block";
	opened = true;
}

function close(event)
{
	save_menu.style.display = "none";
	design_link.style.display = "none";
	opened = false;
}

play_button.addEventListener("click", play);
save_button.addEventListener("click", save);
close_button.addEventListener("click", close);

let canvas = document.getElementById("canvas");
let gl = canvas.getContext("webgl");

let heap_counter = 0;
let heap = {};
let inst;

let param_string = window.location.search;
let params = new URLSearchParams(param_string);

function add_object(obj)
{
	let id = heap_counter++;
	heap[id] = obj;
	return id;
}

function get_object(id)
{
	return heap[id];
}

function make_view(data, size)
{
	return new DataView(inst.exports.memory.buffer, data, size);
}

function make_string(data, size)
{
	let decoder = new TextDecoder();
	return decoder.decode(make_view(data, size));
}

function make_cstring(data)
{
	let size = inst.exports.strlen(data);
	return make_string(data, size);
}

let gl_env = {
	glAttachShader(program, shader) {
		gl.attachShader(get_object(program), get_object(shader));
	},

	glBindBuffer(target, buffer) {
		return gl.bindBuffer(target, get_object(buffer));
	},

	glBufferData(target, size, data, usage) {
		gl.bufferData(target, make_view(data, size), usage);
	},

	glBufferSubData(target, offset, size, data) {
		gl.bufferSubData(target, offset, make_view(data, size));
	},

	glClear(mask) {
		gl.clear(mask);
	},

	glClearColor(red, green, blue, alpha) {
		gl.clearColor(red, green, blue, alpha);
	},

	glCompileShader(shader) {
		gl.compileShader(get_object(shader));
	},

	glCreateBuffer() {
		return add_object(gl.createBuffer());
	},

	glCreateProgram() {
		return add_object(gl.createProgram());
	},

	glCreateShader(type) {
		return add_object(gl.createShader(type));
	},

	glDeleteShader(shader) {
		gl.deleteShader(get_object(shader));
	},

	glDisableVertexAttribArray(index) {
		gl.disableVertexAttribArray(index);
	},

	glDrawArrays(mode, first, count) {
		gl.drawArrays(mode, first, count);
	},

	glDrawElements(mode, count, type, offset) {
		gl.drawElements(mode, count, type, offset);
	},

	glEnableVertexAttribArray(index) {
		gl.enableVertexAttribArray(index);
	},

	glGetAttribLocation(program, name) {
		return gl.getAttribLocation(get_object(program), make_cstring(name));
	},

	glGetProgramParameter(program, pname) {
		return gl.getProgramParameter(get_object(program), pname);
	},

	glGetShaderParameter(shader, pname) {
		return gl.getShaderParameter(get_object(shader), pname);
	},

	glGetUniformLocation(program, name) {
		return add_object(gl.getUniformLocation(get_object(program), make_cstring(name)));
	},

	glLinkProgram(program) {
		gl.linkProgram(get_object(program));
	},

	glShaderSource(shader, count, string, length) {
		let strings = [];
		let lengths = [];
		let string_view = new DataView(inst.exports.memory.buffer, string);

		if (length == 0) {
			for (let i = 0; i < count; i++) {
				let str = string_view.getUint32(i * 4, true);
				let len = inst.exports.strlen(str);
				lengths.push(len);
			}
		} else {
			/* TODO */
		}

		for (let i = 0; i < count; i++) {
			let str = string_view.getUint32(i * 4, true);
			strings.push(make_string(str, lengths[i]));
		}

		let source = strings.join("");
		gl.shaderSource(get_object(shader), source);
	},

	glUniform2f(location, v0, v1) {
		gl.uniform2f(get_object(location), v0, v1);
	},

	glUseProgram(program) {
		gl.useProgram(get_object(program));
	},

	glVertexAttribPointer(index, size, type, normalized, stride, pointer) {
		gl.vertexAttribPointer(index, size, type, normalized, stride, pointer);
	},

	glViewport(x, y, width, height) {
	},

	set_interval(func, delay, arg) {
		return setInterval(inst.exports.call, delay, func, arg);
	},

	clear_interval(id) {
		clearInterval(id);
	},

	print_slice(str, len) {
		console.log(make_string(str, len));
	},

	printf(fmt, args) {
		let fmt_view = new DataView(inst.exports.memory.buffer, fmt);
		let arg_view = new DataView(inst.exports.memory.buffer, args);
		let res = [];
		let i = 0;
		let a = 0;

		while (true) {
			let c = fmt_view.getUint8(i);
			if (c == 37) { // %
				i++;
				let cc = fmt_view.getUint8(i);
				if (cc == 100) { // d
					let d = arg_view.getInt32(a, true);
					res.push(d.toString());
					a += 4;
				} else if (cc == 102) { // f
					a += (-a) & 7;
					let f = arg_view.getFloat64(a, true);
					res.push(f.toString());
					a += 8;
				} else if (cc == 117) { // u
					let u = arg_view.getUint32(a, true);
					res.push(u.toString());
					a += 4;
				}
			} else if (!c) {
				break;
			} else {
				res.push(String.fromCharCode(c));
			}
			i++;
		}

		console.log(res.join(""));
	}
};

let import_object = {
	env: gl_env,
};

function canvas_draw(timestamp)
{
	var width  = canvas.clientWidth;
	var height = canvas.clientHeight;
	if (canvas.width != width || canvas.height != height) {
		canvas.width = width;
		canvas.height = height;
		gl.viewport(0, 0, width, height);
		inst.exports.resize(width, height);
	}
	inst.exports.draw();
	window.requestAnimationFrame(canvas_draw);
}

function to_key(code)
{
	if (code == "Space") return 65;
	if (code == "KeyR") return 27;
	if (code == "KeyM") return 58;
	if (code == "KeyS") return 39;
	if (code == "KeyD") return 40;
	if (code == "KeyU") return 30;
	if (code == "KeyW") return 25;
	if (code == "KeyC") return 54;
	if (code == "ShiftLeft") return 50;
	if (code == "ControlLeft") return 37;
	return 0;
}

function to_button(code)
{
	if (code == 0) return 1;
	return 0;
}

function canvas_keydown(event)
{
	if (opened)
		return;
	inst.exports.key_down(to_key(event.code));
}

function canvas_keyup(event)
{
	if (opened)
		return;
	inst.exports.key_up(to_key(event.code));
}

function canvas_mousedown(event)
{
	inst.exports.button_down(to_button(event.button));
}

function canvas_mouseup(event)
{
	inst.exports.button_up(to_button(event.button));
}

function canvas_mousemove(event)
{
	inst.exports.move(event.offsetX, event.offsetY);
}

function canvas_wheel(event)
{
	inst.exports.scroll(-0.02 * event.deltaY);
}

function alloc_str(str)
{
	let encoder = new TextEncoder();

	let str_uint8 = encoder.encode(str);
	let len = str_uint8.length;
	let mem = inst.exports.malloc(len + 1);
	let mem_uint8 = new Uint8Array(inst.exports.memory.buffer, mem, len + 1);
	mem_uint8.set(str_uint8);
	mem_uint8[len] = 0;

	return mem;
}

function on_text(text)
{
	console.log(text);
	design_link.innerHTML = "fcsim.com/?designId=" + text;
	design_link.style.display = "block";
}

function on_result(response)
{
	let text_promise = response.text();

	text_promise.then(on_text);
}

function save_design(event)
{
	event.preventDefault();

	let data = new FormData(save_form);

	let user = alloc_str(user_id);
	let name = alloc_str(data.get("name"));
	let desc = alloc_str(data.get("description"));

	let xml = inst.exports.export(user, name, desc);
	let len = inst.exports.strlen(xml);

	let xml_str = make_cstring(xml);

	let request_promise = fetch("/fc/saveDesign.php", {
		method: "POST",
		headers: {
			'Content-Type': 'application/x-www-form-urlencoded'
		},
		body: new URLSearchParams({ "xml": xml_str })
	});

	request_promise.then(on_result);
}

function init_module(results)
{
	let module = results[0];
	let buffer = results[1];

	inst = module.instance;

	let buffer_uint8 = new Uint8Array(buffer);
	let len = buffer_uint8.length;
	let mem = inst.exports.malloc(len);
	let mem_uint8 = new Uint8Array(inst.exports.memory.buffer, mem, len);
	mem_uint8.set(buffer_uint8);

	inst.exports.init(mem, buffer_uint8.length);
	inst.exports.resize(canvas.width, canvas.height);
	window.requestAnimationFrame(canvas_draw);
	addEventListener("keydown", canvas_keydown);
	addEventListener("keyup", canvas_keyup);
	canvas.addEventListener("mousedown", canvas_mousedown);
	canvas.addEventListener("mouseup", canvas_mouseup);
	canvas.addEventListener("mousemove", canvas_mousemove);
	canvas.addEventListener("wheel", canvas_wheel);
	save_form.addEventListener("submit", save_design);
}

let module_promise = WebAssembly.instantiateStreaming(
	fetch("fcsim.wasm"), import_object
);

let design_id = params.get('designId');
let level_id = params.get('levelId');

let response_promise = fetch("/fc/retrieveLevel.php", {
	method: "POST",
	headers: {
		'Content-Type': 'application/x-www-form-urlencoded'
	},
	body: new URLSearchParams({
		"id":         design_id ? design_id : level_id,
		"loadDesign": design_id ? "1" : "0",
	})
});

let buffer_promise = response_promise.then((resp) => resp.arrayBuffer());

let module_buffer_promise = Promise.all([module_promise, buffer_promise]);

module_buffer_promise.then(init_module);
