# Embedded code for IoT Project - Measuring Population Density in RIMAC (UCSD Fitness Center)
Above code deployed on ~25 devices (ESP32 Board) for data collection in RIMAC during May 2023.

Each ESP32 board records all nearby bluetooth beaconing devices and their signal strength, then reporting that data to the backend API through HTTP Requests.

# Embedded Code Notes
setup() function runs after every reset of the Arduino board

loop() function is the main loop that repeatedly runs after setup()

# Rest of the Project
- UI Team: https://github.com/Dzhango/RIMAC-data-visualization (Go here to see current production version of final product webpage)
- API Team: https://github.com/NishAru3/RIMAC-Group-Server
- Analytics Team: https://github.com/therab6it/analytics_team

