import 'package:flutter/material.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Sensor Monitoring',
      theme: ThemeData(primarySwatch: Colors.blue),
      home: SensorDashboard(),
      debugShowCheckedModeBanner: false,
    );
  }
}

class SensorDashboard extends StatefulWidget {
  const SensorDashboard({super.key});

  @override
  _SensorDashboardState createState() => _SensorDashboardState();
}

class _SensorDashboardState extends State<SensorDashboard> {
  // Replace with your backend IP
  final String baseUrl = 'http://172.20.10.2:3000/api';
  Map<String, dynamic> sensorData = {};
  bool isLoading = true;

  @override
  void initState() {
    super.initState();
    fetchSensorData();
  }

  // Function to fetch sensor data from the API
  Future<void> fetchSensorData() async {
    setState(() {
      isLoading = true; // Show loading indicator
    });

    try {
      final response = await http.get(Uri.parse('$baseUrl/data'));
      if (response.statusCode == 200) {
        setState(() {
          sensorData = json.decode(response.body); // Update sensorData
        });
      } else {
        throw Exception('Failed to load sensor data');
      }
    } catch (e) {
      print('Error: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Failed to fetch sensor data!')),
      );
    } finally {
      setState(() {
        isLoading = false; // Hide loading indicator
      });
    }
  }

  // Function to control the relay via POST request
  Future<void> controlRelay(String status) async {
    try {
      final response = await http.post(
        Uri.parse('$baseUrl/relay'),
        headers: {'Content-Type': 'application/json'},
        body: json.encode({'status': status}),
      );

      if (response.statusCode == 200) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Relay turned $status successfully!')),
        );
      }
    } catch (e) {
      print('Error controlling relay: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Failed to control relay')),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Sensor Monitoring'),
        actions: [
          // Reload button in AppBar
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: fetchSensorData, // Call the fetchData function on reload
          ),
        ],
      ),
      body: isLoading
          ? const Center(child: CircularProgressIndicator())
          : Padding(
              padding: const EdgeInsets.all(16.0),
              child: SingleChildScrollView(
                // Add SingleChildScrollView to make the content scrollable
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Suhu Max: ${sensorData['suhumax']} °C',
                      style: const TextStyle(
                          fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                    Text(
                      'Suhu Min: ${sensorData['suhumin']} °C',
                      style: const TextStyle(fontSize: 18),
                    ),
                    Text(
                      'Suhu Rata-Rata: ${sensorData['suhurata']} °C',
                      style: const TextStyle(fontSize: 18),
                    ),
                    const SizedBox(height: 20),
                    ElevatedButton(
                      onPressed: () => controlRelay('on'),
                      child: const Text('Turn Relay ON'),
                    ),
                    const SizedBox(height: 10),
                    ElevatedButton(
                      onPressed: () => controlRelay('off'),
                      child: const Text('Turn Relay OFF'),
                    ),
                    const SizedBox(height: 20),
                    const Text(
                      'Kelembapan dan Suhu Tertinggi:',
                      style:
                          TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                    // Display dynamic content as list tiles
                    ...sensorData['nilai_suhu_max_humid_max']
                            ?.map<Widget>((row) => ListTile(
                                  title: Text('Suhu: ${row['suhu']} °C'),
                                  subtitle: Text(
                                      'Humid: ${row['humid']} %, Time: ${row['timestamp']}'),
                                ))
                            ?.toList() ??
                        [],
                  ],
                ),
              ),
            ),
    );
  }
}
