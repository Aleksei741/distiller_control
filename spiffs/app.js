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
    const tempSensorSection = document.getElementById('temp-sensor-settings');

    statusSection.style.display = 'none';
    controlSection.style.display = 'none';
    statisticSection.style.display = 'none';
    controlLedSection.style.display = 'none';
    tempSensorSection.style.display = 'none';
    
    if (current == "control") {
        statusSection.style.display = 'block';
        controlSection.style.display = 'block';
        controlLedSection.style.display = 'block';
    } else if (current == "statistic") {
        statisticSection.style.display = 'block';
    } else if (current == "tempsensor") {
        tempSensorSection.style.display = 'block';
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

    if (pass.length < 8) {
        alert("Ошибка: пароль должен быть не менее 8 символов");
        return; // Прерываем выполнение функции
    }

    fetch(`/api/wifi/ap?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`)
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
async function scanWifi() {
    // 1. Стартуем сканирование
    console.log("Starting WiFi scan...");
    await fetch("/api/wifi/sta/scan/start");

    // 2. Ждём завершения
    console.log("Waiting for scan to complete...");

    for (;;) 
    {
        await new Promise(r => setTimeout(r, 300)); // 300ms пауза между запросами

        let resp = await fetch("/api/wifi/sta/scan/isdone");
        let data = await resp.json();

        if (data.wifi_sta_isdone === true) 
        {
            console.log("Scan completed");
            break;
        }
    }

    // 3. Получаем список сетей
    console.log("Fetching scan results...");
    let res = await fetch("/api/wifi/sta/scan/result");
    let aps = await res.json();

    console.log("Scan result:", aps);

    return aps;
}

//Запуск сканирования
async function scanNetworks() {
    let aps = await scanWifi(); // получаем список сетей

    const list = document.getElementById("wifi-list");
    list.innerHTML = ""; // очищаем список

    aps.forEach(ap => {
        // создаём элемент списка
        const item = document.createElement("li");

        // контейнер для текста SSID
        const ssidSpan = document.createElement("span");
        ssidSpan.className = "ssid";
        ssidSpan.textContent = ap.ssid;

        // графический индикатор уровня сигнала
        const rssiBar = document.createElement("div");
        rssiBar.className = "rssi-bar";

        // нормализация RSSI -100..0 -> 0..100%
        let signalStrength = Math.min(Math.max(ap.rssi + 100, 0), 100);
        rssiBar.style.background = `linear-gradient(to right, green ${signalStrength}%, #ccc ${signalStrength}%)`;

        // добавляем элементы в li
        item.appendChild(ssidSpan);
        item.appendChild(rssiBar);

        // обработчик клика: заполняем поле ввода SSID
        item.addEventListener("click", () => {
            document.getElementById("sta-ssid").value = ap.ssid;
        });

        // добавляем элемент в список
        list.appendChild(item);
    });
}

//==========================================================================
function TempSensorSettings(type) {    
    if(type == "cube") {
        fetch(`/api/tempsensor/rom/cube`)
            .then(res => res.json())
    }
    else if(type == "column") {
        fetch(`/api/tempsensor/rom/column`)
            .then(res => res.json())
    }
}

// При загрузке страницы
document.addEventListener('DOMContentLoaded', function() 
{
    updateTempSensorROM();    
    setInterval(updateTempSensorROM, 1000);
});

async function getTempROM() {
    try 
    {
        const response = await fetch('/api/tempsensor/rom');
        if (!response.ok) throw new Error(`Ошибка: ${response.status}`);
        const data = await response.json();
        return data;
    } 
    catch (error) 
    {
        console.error('Ошибка получения ROM:', error);
        return null;
    }
}

// Обновить отображение статуса LED
async function updateTempSensorROM() {
    const panel = document.getElementById('temp-sensor-settings');
    if (!panel || panel.offsetParent === null) return; // элемент реально скрыт
    

    const rom = await getTempROM();
    console.log('Обновление ROM датчиков температуры:', rom);
    if (!rom) return; // ошибка запроса

    document.getElementById('temp-sensor-kube-rom').innerText = rom.kube;
    document.getElementById('temp-sensor-column-rom').innerText = rom.column;
}
