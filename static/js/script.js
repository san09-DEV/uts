// Function to fetch data from MongoDB via the Flask API
async function fetchSensorData() {
    try {
        const response = await fetch('/get_data');
        const data = await response.json();

        if (data.length > 0) {
            const suhumax = data[0].suhumax;
            const suhumin = data[0].suhumin;
            const suhurata2 = data[0].suhurata2;

            document.getElementById("temp-max").textContent = suhumax;
            document.getElementById("temp-min").textContent = suhumin;
            document.getElementById("temp-avg").textContent = suhurata2;

            const temperatureData = data.map(item => item.nilaisuhuhumid.map(val => val.suhu)).flat();
            const humidityData = data.map(item => item.nilaisuhuhumid.map(val => val.humid)).flat();
            const timestamps = data.map(item => item.nilaisuhuhumid.map(val => new Date(val.timestamp).toLocaleTimeString())).flat();

            // Temperature Chart
            const tempCtx = document.getElementById("temperatureChart").getContext("2d");
            new Chart(tempCtx, {
                type: "line",
                data: {
                    labels: timestamps,
                    datasets: [
                        {
                            label: "Temperature (°C)",
                            data: temperatureData,
                            borderColor: "#FF6384",
                            backgroundColor: "rgba(255, 99, 132, 0.2)",
                            fill: true,
                            tension: 0.4 // Smooth the line
                        }
                    ]
                },
                options: {
                    responsive: true,
                    plugins: {
                        title: {
                            display: true,
                            text: 'Temperature over Time',
                            font: {
                                size: 18,
                                weight: 'bold'
                            }
                        },
                        legend: {
                            display: true,
                            position: 'top'
                        }
                    },
                    scales: {
                        y: {
                            title: {
                                display: true,
                                text: 'Temperature (°C)',
                                color: '#FF6384'
                            }
                        },
                        x: {
                            title: {
                                display: true,
                                text: 'Time',
                                color: '#888'
                            }
                        }
                    }
                }
            });

            // Humidity Chart
            const humidCtx = document.getElementById("humidityChart").getContext("2d");
            new Chart(humidCtx, {
                type: "line",
                data: {
                    labels: timestamps,
                    datasets: [
                        {
                            label: "Humidity (%)",
                            data: humidityData,
                            borderColor: "#36A2EB",
                            backgroundColor: "rgba(54, 162, 235, 0.2)",
                            fill: true,
                            tension: 0.4 // Smooth the line
                        }
                    ]
                },
                options: {
                    responsive: true,
                    plugins: {
                        title: {
                            display: true,
                            text: 'Humidity over Time',
                            font: {
                                size: 18,
                                weight: 'bold'
                            }
                        },
                        legend: {
                            display: true,
                            position: 'top'
                        }
                    },
                    scales: {
                        y: {
                            title: {
                                display: true,
                                text: 'Humidity (%)',
                                color: '#36A2EB'
                            }
                        },
                        x: {
                            title: {
                                display: true,
                                text: 'Time',
                                color: '#888'
                            }
                        }
                    }
                }
            });
        } else {
            console.log("No data found");
        }
    } catch (error) {
        console.error("Error fetching sensor data:", error);
    }
}

// Load data when the page is loaded
document.addEventListener("DOMContentLoaded", fetchSensorData);

// Function to update the current date and time in real-time
function updateCurrentDate() {
    const dateElement = document.getElementById("currentDate");

    setInterval(() => {
        const now = new Date();
        const formattedDate = now.toLocaleString("en-GB", {
            weekday: 'long', 
            year: 'numeric', 
            month: 'long', 
            day: 'numeric',
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit'
        });

        dateElement.textContent = formattedDate;
    }, 1000);
}

// Call the function when the page loads
document.addEventListener("DOMContentLoaded", updateCurrentDate);