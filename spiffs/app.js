//==========================================================================
//Анимация
let y = 360;
function animateVapor() {
    y -= 1;
    if (y < 140) y = 360;
    document.getElementById('vapor').setAttribute('cy', y);
    requestAnimationFrame(animateVapor);
}
animateVapor();

//==========================================================================
// Режим управления
document.querySelectorAll('input[name="mode"]').forEach(el => {
    el.addEventListener('change', () => {
        if (el.value === 'manual') {
            document.getElementById('manual-control').style.display = 'block';
            document.getElementById('auto-control').style.display = 'none';
        } else {
            document.getElementById('manual-control').style.display = 'none';
            document.getElementById('auto-control').style.display = 'block';
        }
    });
});

document.getElementById('manual-power').addEventListener('input', (e) => {
    document.getElementById('manual-power-value').innerText = e.target.value + '%';
});




// Управление ТЭНом
function setManualPower() {
    const power = document.getElementById('manual-power').value;
    fetch(`/api/ten/manual?power=${power}`)
        .then(res => res.json())
        .then(alert);
}

function setAutoStage() {
    const stage = document.getElementById('auto-stage').value;
    fetch(`/api/ten/auto?stage=${stage}`)
        .then(res => res.json())
        .then(alert);
}

//==========================================================================
// LED
// Функция для запроса статуса LED
async function getLEDStatus() {
    try 
    {
        const response = await fetch('/api/led/status');
        if (!response.ok) throw new Error(`Ошибка: ${response.status}`);
        const data = await response.json();
        return data.led; // true или false
    } 
    catch (error) 
    {
        console.error('Ошибка получения статуса LED:', error);
        return null;
    }
}

// При загрузке страницы
document.addEventListener('DOMContentLoaded', function() 
{
    updateLEDIndicator(null);
    updateLEDStatusDisplay();
    
    setInterval(updateLEDStatusDisplay, 5000);
});

// Обновить отображение статуса LED
async function updateLEDStatusDisplay() 
{
    const panel = document.getElementById('control-led-section');
    if(panel.style.display === 'none') return;
    const status = await getLEDStatus();
    updateLEDIndicator(status);
    console.log('Статус LED:', status);
}

// Функция для переключения LED
async function toggleLED() {
    try 
    {
        updateLEDIndicator(null);        
        const response = await fetch('/api/led/toggle');
        if (!response.ok) throw new Error(`Ошибка: ${response.status}`);
        const data = await response.json();
        
        // Обновляем индикатор
        updateLEDIndicator(data.led);
        return data;
    } 
    catch (error) 
    {
        console.error('Ошибка при переключении LED:', error);
        // При ошибке пытаемся получить актуальный статус
        updateLEDStatusDisplay();
        alert('Не удалось переключить LED');
    }
}

// Функция для обновления визуального индикатора LED
function updateLEDIndicator(status) 
{
    const indicator = document.getElementById('led-status');

    if (!indicator) return;

    if (status === true) 
    {
        indicator.setAttribute("fill", "#4CAF50"); // Зеленый
    } 
    else if (status === false) 
    {
        indicator.setAttribute("fill", "#f44336"); // Красный
    } 
    else 
    {
        indicator.setAttribute("fill", "#FFA500"); // Оранжевый
    }
}

//==========================================================================
// Статистика
// Инициализация графика
const ctx = document.getElementById('chart').getContext('2d');
const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            {
                label: 'Температура куба',
                borderColor: 'red',
                fill: false,
                data: []
            },
            {
                label: 'Температура колонны',
                borderColor: 'blue',
                fill: false,
                data: []
            },
            {
                label: 'Мощность ТЭНа',
                borderColor: 'green',
                fill: false,
                data: []
            }
        ]
    },
    options: {
        responsive: true,
        animation: false,
        scales: {
            x: { title: { display: true, text: 'Время' } },
            y: { title: { display: true, text: 'Значение' }, min: 0 }
        }
    }
});

// Периодическое обновление данных
/*setInterval(() => {
    fetch('/api/status')
        .then(res => res.json())
        .then(data => {
            document.getElementById('temp-cube').innerText = data.tempCube;
            document.getElementById('temp-column').innerText = data.tempColumn;
            document.getElementById('ten-power').innerText = data.tenPower;

            const time = new Date().toLocaleTimeString();
            chart.data.labels.push(time);
            chart.data.datasets[0].data.push(data.tempCube);
            chart.data.datasets[1].data.push(data.tempColumn);
            chart.data.datasets[2].data.push(data.tenPower);

            // Ограничим длину графика
            if(chart.data.labels.length > 50){
                chart.data.labels.shift();
                chart.data.datasets.forEach(ds => ds.data.shift());
            }

            chart.update();
        });
}, 1000);*/

//==========================================================================
// Функция для переключения между секциями
function switchSections(current = "control") {
    const statusSection = document.getElementById('status-section');
    const controlSection = document.getElementById('control-section');
    const statisticSection = document.getElementById('statistic-section');
    const controlLedSection = document.getElementById('control-led-section');

    statusSection.style.display = 'none';
    controlSection.style.display = 'none';
    statisticSection.style.display = 'none';
    controlLedSection.style.display = 'none';
    
    if (current == "control") {
        statusSection.style.display = 'block';
        controlSection.style.display = 'block';
        controlLedSection.style.display = 'block';
    } else if (current == "statistic") {
        statisticSection.style.display = 'block';
    }
}

//==========================================================================
// Всплывающее меню в header
document.addEventListener("click", function (e) {    
    // Ищем ближайшую кнопку
    const clickedButton = e.target.closest('button');
    
    if (clickedButton) {
        // Ищем ближайший .btn-wrap, содержащий эту кнопку
        const wrap = clickedButton.closest('.btn-wrap');
        
        if (wrap) {
            // Проверяем, есть ли у этого wrap подменю
            const submenu = wrap.querySelector('.submenu');
            
            if (submenu) {
                // Закрываем другие открытые меню
                document.querySelectorAll(".btn-wrap.show").forEach(el => {
                    if (el !== wrap) el.classList.remove("show");
                });
                
                // Переключаем текущее меню
                wrap.classList.toggle("show");
                e.stopPropagation();
                return;
            }
        }
    }

    // Клик внутри подменю — не закрываем
    if (e.target.closest(".submenu")) {
        return;
    }

    // Клик вне — закрываем все меню
    document.querySelectorAll(".btn-wrap.show")
        .forEach(el => el.classList.remove("show"));
});
//==========================================================================
// Звуковой сигнал
function beep(frequency = 440, duration = 200) {
    const ctx = new (window.AudioContext || window.webkitAudioContext)();
    const oscillator = ctx.createOscillator();
    oscillator.type = 'square'; // тип сигнала: sine, square, sawtooth, triangle
    oscillator.frequency.setValueAtTime(frequency, ctx.currentTime);
    oscillator.connect(ctx.destination);
    oscillator.start();
    oscillator.stop(ctx.currentTime + duration / 1000);
}

//==========================================================================
// Функции для работы с модалками Wi-Fi AP
// Показываем модалку
function showAPmodal() {
    document.getElementById('wifi-AP-modal').style.display = 'block';
}

// Скрываем модалку
function hideAPmodal() {
    document.getElementById('wifi-AP-modal').style.display = 'none';
}

// Применение настройки AP
function setupAP() {
    const ssid = document.getElementById('ap-ssid').value;
    const pass = document.getElementById('ap-pass').value;
    fetch(`/api/wifi/ap?ssid=${ssid}&pass=${pass}`)
        .then(res => res.json())
        .then(alert);
    hideAPmodal();
}


//==========================================================================
// Функции для работы с модалками Wi-Fi STA
// Показываем модалку
function showSTAmodal() {
    document.getElementById('wifi-STA-modal').style.display = 'block';
}

// Скрываем модалку
function hideSTAmodal() {
    document.getElementById('wifi-STA-modal').style.display = 'none';
}

// Пример подключения
function connectSTA() {
    const ssid = document.getElementById('sta-ssid').value;
    const pass = document.getElementById('sta-pass').value;
    fetch(`/api/wifi/sta?ssid=${ssid}&pass=${pass}`)
        .then(res => res.json())
        .then(alert);
    hideSTAmodal();
}

// Сканирование сетей через ESP32
function scanNetworks() {
    const listEl = document.getElementById('wifi-list');
    listEl.innerHTML = '<li>Сканирование...</li>';

    // Пример fetch: ESP32 должен отдавать JSON с массивом сетей
    fetch('/api/wifi/sta/scan') // URL на ESP32, который возвращает [{ssid: "MyWiFi", rssi: -50}, ...]
        .then(response => response.json())
        .then(data => {
            listEl.innerHTML = '';
            data.forEach(net => {
                const li = document.createElement('li');
                li.textContent = `${net.ssid} (${net.rssi} dBm)`;
                li.addEventListener('click', () => {
                    document.getElementById('sta-ssid').value = net.ssid;
                });
                listEl.appendChild(li);
            });
        })
        .catch(err => {
            listEl.innerHTML = '<li>Ошибка сканирования</li>';
            console.error(err);
        });
}

// [
//   {"ssid": "HomeWiFi", "rssi": -45},
//   {"ssid": "Guest", "rssi": -70},
//   {"ssid": "CafeNet", "rssi": -80}
// ]
//==========================================================================