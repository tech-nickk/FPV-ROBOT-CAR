extern String WiFiAddr;

const char WEBPAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0">
    <style>

        // #screeen_backgroud{
        //   background-color: #818589;
        // }
        #joystick-area {
            width: 200px;
            height: 200px;
            position: relative;
            background-color: #ccc;
            border-radius: 50%;
            margin: 20px auto;
        }
        button {
            border-radius: 15px;
            width: 140px;
            height: 40px;
            margin: 5px;
            font-weight: bold;
            cursor: pointer;
        }
        #lightButton {
            background-color: lightblue;
        }
        #captureButton {
            background-color: #4CAF50;
            color: white;
        }
        #recordButton {
            background-color: #f44336;
            color: white;
        }

        /* Custom Joystick CSS */
        .nipple {
            position: absolute;
            width: 100px;
            height: 100px;
            background-color: purple;
            border-radius: 50%;
            margin: -50px 0 0 -50px;
        }
    </style>
</head>
<body id="screeen_backgroud">
    <p align="center">
        <img src="http://{IP address}:81/stream" style="width:100%; object-fit:cover;"> //Replace with your actual IP {IP address}
    </p>

    <div id="joystick-area"></div>

    <p align="center">
        <button id="lightButton" onclick="toggleLight()">Light OFF</button>
        <button id="captureButton" onclick="captureImage()">Capture Image</button>
        <button id="recordButton" onclick="toggleRecording()">Start Recording</button>
    </p>

    <!-- Embedded NippleJS -->
    <script>
        document.addEventListener('DOMContentLoaded', function() {
            var joystickArea = document.getElementById('joystick-area');
            var nipple = document.createElement('div');
            nipple.className = 'nipple';
            joystickArea.appendChild(nipple);

            var startPos = { x: joystickArea.offsetWidth / 2, y: joystickArea.offsetHeight / 2 };
            nipple.style.left = startPos.x + 'px';
            nipple.style.top = startPos.y + 'px';

            var joystick = {
                x: startPos.x,
                y: startPos.y,
                isMoving: false
            };

            joystickArea.addEventListener('touchstart', function(event) {
                joystick.isMoving = true;
            });

            joystickArea.addEventListener('touchmove', function(event) {
                if (joystick.isMoving) {
                    var touch = event.touches[0];
                    var rect = joystickArea.getBoundingClientRect();
                    joystick.x = touch.clientX - rect.left;
                    joystick.y = touch.clientY - rect.top;
                    nipple.style.left = joystick.x + 'px';
                    nipple.style.top = joystick.y + 'px';

                    var direction = getDirection(startPos, { x: joystick.x, y: joystick.y });
                    if (direction) {
                        getsend(direction);
                    }
                }
            });

            joystickArea.addEventListener('touchend', function(event) {
                joystick.isMoving = false;
                nipple.style.left = startPos.x + 'px';
                nipple.style.top = startPos.y + 'px';
                getsend('stop');
            });

            function getDirection(start, end) {
                var dx = end.x - start.x;
                var dy = end.y - start.y;

                var angle = Math.atan2(dy, dx) * 180 / Math.PI;

                if (angle >= -45 && angle < 45) return 'right';
                if (angle >= 45 && angle < 135) return 'back';
                if (angle >= -135 && angle < -45) return 'go';
                return 'left';
            }

            function getsend(arg) {
                var xhttp = new XMLHttpRequest();
                xhttp.open('GET', arg + '?' + new Date().getTime(), true);
                xhttp.send();
            }

            document.getElementById('lightButton').addEventListener('click', function() {
                toggleLight();
            });

            document.getElementById('captureButton').addEventListener('click', function() {
                captureImage();
            });

            document.getElementById('recordButton').addEventListener('click', function() {
                toggleRecording();
            });

            var isLightOn = false;
            var isRecording = false;

            function toggleLight() {
                isLightOn = !isLightOn;
                var lightButton = document.getElementById('lightButton');
                if (isLightOn) {
                    getsend('ledon');
                    lightButton.style.backgroundColor = 'pink';
                    lightButton.textContent = 'Light ON';
                } else {
                    getsend('ledoff');
                    lightButton.style.backgroundColor = 'lightblue';
                    lightButton.textContent = 'Light OFF';
                }
            }

            function captureImage() {
                getsend('capture');
                alert('Image captured!');
            }

            function toggleRecording() {
                isRecording = !isRecording;
                var recordButton = document.getElementById('recordButton');
                if (isRecording) {
                    getsend('startrecord');
                    recordButton.textContent = 'Stop Recording';
                    recordButton.style.backgroundColor = '#e57373';
                } else {
                    getsend('stoprecord');
                    recordButton.textContent = 'Start Recording';
                    recordButton.style.backgroundColor = '#f44336';
                }
            }
        });
    </script>
</body>
</html>
)=====";
