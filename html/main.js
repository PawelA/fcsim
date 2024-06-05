"use strict";

let canvas = document.getElementById("canvas");
let gl = canvas.getContext("webgl");

let heap_counter = 0;
let heap = {};
let inst;

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

	glDrawArrays(mode, first, count) {
		gl.drawArrays(mode, first, count);
	},

	glDrawElements(mode, count, type, offset) {
		gl.drawElements(mode, count, type, offset);
	},

	glEnableVertexAttribArray(index) {
		gl.enableVertexAttribArray(index);
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

function canvas_keydown(event)
{
	inst.exports.key_down(65);
}

function canvas_keyup(event)
{
	inst.exports.key_up(65);
}

function canvas_mousedown(event)
{
	inst.exports.button_down(1);
}

function canvas_mouseup(event)
{
	inst.exports.button_up(1);
}

function canvas_mousemove(event)
{
	inst.exports.move(event.offsetX, event.offsetY);
}

function canvas_wheel(event)
{
	inst.exports.scroll(-0.05 * event.deltaY);
}

function module_instantiated(module)
{
	inst = module.instance;
	inst.exports.init();
	inst.exports.resize(canvas.width, canvas.height);
	window.requestAnimationFrame(canvas_draw);
	addEventListener("keydown", canvas_keydown);
	addEventListener("keyup", canvas_keyup);
	canvas.addEventListener("mousedown", canvas_mousedown);
	canvas.addEventListener("mouseup", canvas_mouseup);
	canvas.addEventListener("mousemove", canvas_mousemove);
	canvas.addEventListener("wheel", canvas_wheel);
}

let module_promise = WebAssembly.instantiateStreaming(
	fetch("fcsim.wasm"), import_object
);

module_promise.then(module_instantiated);