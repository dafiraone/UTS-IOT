const express = require('express');
const bodyParser = require('body-parser');
const mqtt = require('mqtt');
const mysql = require('mysql2');
const axios = require("axios")
const cors = require('cors');

const app = express();
const PORT = 3000;

// Middleware
app.use(bodyParser.json());
app.use(cors());

// Koneksi ke MySQL
const db = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: '', // Ganti dengan password MySQL Anda
    database: 'utsiot_152022003',
});

db.connect(err => {
    if (err) {
        console.error('Koneksi ke database gagal:', err);
        return;
    }
    console.log('Koneksi ke database berhasil');
});

// Koneksi ke MQTT broker
const MQTT_BROKER = 'mqtt://broker.hivemq.com';
const mqttClient = mqtt.connect(`${MQTT_BROKER}`);
const MQTT_TOPIC = "152022003_UTS";

let latestData = null;

mqttClient.on('connect', () => {
    console.log('Terhubung ke broker MQTT');
    mqttClient.subscribe(`${MQTT_TOPIC}`, err => {
        if (!err) {
            console.log(`Berlangganan ke topik ${MQTT_TOPIC}`);
        }
    });
});

mqttClient.on('message', (topic, message) => {
    if (topic === `${MQTT_TOPIC}`) {
        const data = JSON.parse(message.toString());

        if (latestData !== data) {
            // Simpan ke database
            const query = 'INSERT INTO sensor_data (suhu, humid, kecerahan) VALUES (?, ?, ?)';
            db.query(query, [data.suhu, data.kelembapan, data.kecerahan], (err, results) => {
                if (err) {
                    console.error('Gagal menyimpan data ke database:', err);
                } else {
                    console.log('Data berhasil disimpan:', results.insertId);
                }
            });
        }
        latestData = data;
    }
});

const ESP32_IP = '172.20.10.3'; // Ganti dengan IP ESP32

// Endpoint untuk kontrol relay
app.post('/api/relay', async (req, res) => {
    const { status } = req.body; // Status bisa "on" atau "off"

    if (status !== 'on' && status !== 'off') {
        return res.status(400).json({ error: 'Status harus "on" atau "off"' });
    }

    try {
        const url = `http://${ESP32_IP}/relay/${status}`;
        const response = await axios.get(url); // Kirim request ke ESP32
        res.json({ message: response.data }); // Kirim respons dari ESP32 ke client
    } catch (error) {
        console.error('Gagal mengontrol relay:', error.message);
        res.status(500).json({ error: 'Gagal mengontrol relay' });
    }
});


// Endpoint untuk JSON data sensor
app.get('/api/data', (req, res) => {
    const querySuhu = 'SELECT MAX(suhu) AS suhuMax, MIN(suhu) AS suhuMin, AVG(suhu) AS suhuRata FROM sensor_data';
    const queryDetail = `
        SELECT id AS idx, suhu, humid, kecerahan, timestamp 
        FROM sensor_data 
        WHERE suhu = (SELECT MAX(suhu) FROM sensor_data) 
        OR humid = (SELECT MAX(humid) FROM sensor_data)
    `;
    const queryMonthYear = `
        SELECT DISTINCT DATE_FORMAT(timestamp, '%m-%Y') AS month_year 
        FROM sensor_data 
        WHERE suhu = (SELECT MAX(suhu) FROM sensor_data) 
        OR humid = (SELECT MAX(humid) FROM sensor_data)
    `;

    db.query(querySuhu, (err, suhuResults) => {
        if (err) {
            return res.status(500).json({ error: 'Gagal mengambil data suhu' });
        }

        db.query(queryDetail, (err, detailResults) => {
            if (err) {
                return res.status(500).json({ error: 'Gagal mengambil data detail' });
            }

            db.query(queryMonthYear, (err, monthYearResults) => {
                if (err) {
                    return res.status(500).json({ error: 'Gagal mengambil data bulan dan tahun' });
                }

                const response = {
                    suhumax: suhuResults[0].suhuMax,
                    suhumin: suhuResults[0].suhuMin,
                    suhurata: parseFloat(suhuResults[0].suhuRata.toFixed(2)),
                    nilai_suhu_max_humid_max: detailResults.map(row => ({
                        idx: row.idx,
                        suhu: row.suhu,
                        humid: row.humid,
                        kecerahan: row.kecerahan,
                        timestamp: row.timestamp,
                    })),
                    month_year_max: monthYearResults.map(row => ({
                        month_year: row.month_year,
                    })),
                };

                res.json(response);
            });
        });
    });
});

// Jalankan server
app.listen(PORT, () => {
    console.log(`Server berjalan di http://localhost:${PORT}`);
});
