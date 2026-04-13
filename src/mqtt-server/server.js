const express = require("express");
const mqtt = require("mqtt");
const fs = require("fs");
const cors = require("cors");

const app = express();
app.use(cors());

const port = 3000;

// 🔥 FIX ĐƯỜNG DẪN FILE
const DATA_FILE = __dirname + "/data.json";

// 🔥 nếu chưa có file → tạo
if (!fs.existsSync(DATA_FILE)) {
  fs.writeFileSync(DATA_FILE, "[]");
}

// ================= MQTT =================
const client = mqtt.connect(
  "mqtts://4477cf883375485e9156af4ed6ab121b.s1.eu.hivemq.cloud:8883",
  {
    username: "hung00",
    password: "0565241132aA",
  }
);

// ================= LOAD DATA =================
let history = [];

try {
  history = JSON.parse(fs.readFileSync(DATA_FILE));
  console.log("Đã load dữ liệu cũ:", history.length);
} catch (e) {
  console.log("Lỗi đọc file → reset data");
  history = [];
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

    // thêm dữ liệu
    history.push(data);

    // giữ tối đa 100 mẫu
    if (history.length > 100) {
      history.shift();
    }

    // 🔥 GHI FILE ĐÚNG CHỖ
    fs.writeFileSync(DATA_FILE, JSON.stringify(history, null, 2));

    console.log("Đã lưu dữ liệu vào file");

  } catch (e) {
    console.log("Lỗi parse JSON:", e.message);
  }
});

// ================= API =================

// test server
app.get("/", (req, res) => {
  res.send("Server MQTT + Data đang chạy OK");
});

// API lấy dữ liệu
app.get("/data", (req, res) => {
  res.json(history);
});

// ================= START =================
app.listen(port, () => {
  console.log(`Server chạy tại http://localhost:${port}`);
});