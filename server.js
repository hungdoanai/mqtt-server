const express = require("express");
const mqtt = require("mqtt");
const fs = require("fs");
const cors = require("cors");

const app = express();
app.use(cors());

const port = 3000;

// ================= MQTT =================
const client = mqtt.connect("mqtts://4477cf883375485e9156af4ed6ab121b.s1.eu.hivemq.cloud:8883", {
  username: "hung00",
  password: "0565241132aA"
});

// ================= LOAD DATA =================
let history = [];

// load file nếu có
if (fs.existsSync("data.json")) {
  try {
    history = JSON.parse(fs.readFileSync("data.json"));
    console.log("Đã load dữ liệu cũ:", history.length);
  } catch (e) {
    console.log("Lỗi đọc file data.json");
    history = [];
  }
}

// ================= MQTT CONNECT =================
client.on("connect", () => {
  console.log("MQTT đã kết nối");

  client.subscribe("esp32/sensor", (err) => {
    if (!err) {
      console.log("Subscribe thành công esp32/sensor");
    } else {
      console.log("Lỗi subscribe:", err);
    }
  });
});

// ================= MQTT ERROR =================
client.on("error", (err) => {
  console.log("MQTT lỗi:", err);
});

// ================= NHẬN DỮ LIỆU =================
client.on("message", (topic, message) => {
  try {
    const data = JSON.parse(message.toString());

    console.log("Nhận từ MQTT:", data);

    // lưu vào mảng
    history.push(data);

    // giữ tối đa 100 mẫu
    if (history.length > 100) history.shift();

    // lưu file
    fs.writeFileSync("data.json", JSON.stringify(history, null, 2));

    console.log("Đã lưu dữ liệu");

  } catch (e) {
    console.log("Lỗi parse JSON");
  }
});

// ================= API =================

// test server
app.get("/", (req, res) => {
  res.send("Server MQTT + Data đang chạy OK");
});

// 🔥 API QUAN TRỌNG (fix lỗi của bạn)
app.get("/data", (req, res) => {
  res.json(history);
});

// ================= START =================
app.listen(port, () => {
  console.log(`Server chạy tại http://localhost:${port}`);
});