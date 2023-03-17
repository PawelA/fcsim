/*
const params = new URLSearchParams(window.location.search);
const design_id = params.get("designId");

if (!design_id)
	throw "No designId";

fetch("http://fantasticcontraption.com/retrieveLevel.php", {
	method: "POST",
	body: JSON.stringify({
		id: design_id,
		loadDesign: 1
	})
}).then((res) => {
	console.log(res);
});
*/

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

{
	const r = 0.529 * 256;
	const g = 0.741 * 256;
	const b = 0.945 * 256;
	canvas.style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
}

ctx.translate(300, 400);
ctx.scale(0.4, 0.4)

async function main() {
	const import_object = {
		'env': { 'log': console.log }
	};
	const module = await WebAssembly.instantiateStreaming(
		fetch("fcsim.wasm"), import_object
	);
	const instance = module.instance;

	instance.exports.memory.grow(100);

	const xml_res = await fetch("galois.xml");
	const xml_buf = await xml_res.arrayBuffer();
	const xml = new Uint8Array(xml_buf);

	const xml_c_ptr = instance.exports.malloc(xml.length + 1);
	const xml_c_arr = new Uint8Array(
		instance.exports.memory.buffer,
		xml_c_ptr,
		xml.length + 1
	);
	xml_c_arr.set(xml);

	const arena_c_ptr = instance.exports.malloc(72);
	const res = instance.exports.fcsim_read_xml(xml_c_ptr, arena_c_ptr);
	if (res)
		return;
	const handle_c_ptr = instance.exports.fcsim_new(arena_c_ptr);
	//instance.exports.fcsim_step(handle_c_ptr);

	const view = new DataView(instance.exports.memory.buffer);

	const blocks_c_ptr = view.getUint32(arena_c_ptr, true);
	const block_cnt = view.getUint32(arena_c_ptr + 4, true);
	const build_x = view.getFloat64(arena_c_ptr + 8, true);
	const build_y = view.getFloat64(arena_c_ptr + 16, true);
	const build_w = view.getFloat64(arena_c_ptr + 24, true);
	const build_h = view.getFloat64(arena_c_ptr + 32, true);
	const goal_x = view.getFloat64(arena_c_ptr + 40, true);
	const goal_y = view.getFloat64(arena_c_ptr + 48, true);
	const goal_w = view.getFloat64(arena_c_ptr + 56, true);
	const goal_h = view.getFloat64(arena_c_ptr + 64, true);

	const build_l = build_x - build_w / 2;
	const build_b = build_y - build_h / 2;
	{
		const r = 0.737 * 256;
		const g = 0.859 * 256;
		const b = 0.976 * 256;
		ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
	}
	ctx.fillRect(build_l, build_b, build_w, build_h);
	
	const goal_l = goal_x - goal_w / 2;
	const goal_b = goal_y - goal_h / 2;
	{
		const r = 0.945 * 256;
		const g = 0.569 * 256;
		const b = 0.569 * 256;
		ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
	}
	ctx.fillRect(goal_l, goal_b, goal_w, goal_h);

	for (let i = 0; i < block_cnt; i++) {
		const off = blocks_c_ptr + i * 64;
		const type = view.getUint32(off, true);
		const id = view.getInt32(off + 4, true);
		const x = view.getFloat64(off + 8, true);
		const y = view.getFloat64(off + 16, true);
		const w = view.getFloat64(off + 24, true);
		const h = view.getFloat64(off + 32, true);
		const angle = view.getFloat64(off + 40, true);
		const joint_cnt = view.getUint32(off + 56, true);
		let joints = [];
		for (let j = 0; j < joint_cnt; j++)
			joints.push(view.getInt32(off + 48 + j * 4, true));

		console.log(type, id, x, y, w, h, angle, joints);

		const colors = [
			{ 'c': 0, 'r': 0.000, 'g': 0.745, 'b': 0.004 },
			{ 'c': 1, 'r': 0.000, 'g': 0.745, 'b': 0.004 },
			{ 'c': 0, 'r': 0.976, 'g': 0.855, 'b': 0.184 },
			{ 'c': 1, 'r': 0.976, 'g': 0.537, 'b': 0.184 },
			{ 'c': 0, 'r': 1.000, 'g': 0.400, 'b': 0.400 },
			{ 'c': 1, 'r': 1.000, 'g': 0.400, 'b': 0.400 },
			{ 'c': 1, 'r': 0.537, 'g': 0.980, 'b': 0.890 },
			{ 'c': 1, 'r': 1.000, 'g': 0.925, 'b': 0.000 },
			{ 'c': 1, 'r': 1.000, 'g': 0.800, 'b': 0.800 },
			{ 'c': 0, 'r': 0.000, 'g': 0.000, 'b': 1.000 },
			{ 'c': 0, 'r': 0.420, 'g': 0.204, 'b': 0.000 }
		];
		const circ = colors[type].c;
		const r = colors[type].r * 256;
		const g = colors[type].g * 256;
		const b = colors[type].b * 256;
		console.log(r, g, b);
		ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;

		ctx.beginPath();
		if (circ) {
			ctx.arc(x, y, w / 2, 0, Math.PI * 2, true);
		} else {
			const sina = Math.sin(angle);
			const cosa = Math.cos(angle);
			const wc = Math.max(4, w) * cosa / 2;
			const ws = Math.max(4, w) * sina / 2;
			const hc = Math.max(4, h) * cosa / 2;
			const hs = Math.max(4, h) * sina / 2;
			ctx.moveTo( wc - hs + x,  ws + hc + y);
			ctx.lineTo(-wc - hs + x, -ws + hc + y);
			ctx.lineTo(-wc + hs + x, -ws - hc + y);
			ctx.lineTo( wc + hs + x,  ws - hc + y);
		}
		ctx.fill();
	}

	console.log(blocks_c_ptr, block_cnt);
}

main();
