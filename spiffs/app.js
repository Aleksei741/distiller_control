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
// Функции для работы с управлением
let isUserSetManualPower = true;
let isPauseManualPower = false; 
let isUserSetMode = false;
let isPauseSetMode = false;

//Переключатель режима управления
document.querySelectorAll('input[name="mode"]').forEach(el => 
{
    document.getElementById('manual-control').style.display = 'none';
    document.getElementById('auto-control').style.display = 'none';

    el.addEventListener('change', () =>
    {
        if (el.value === 'manual') 
        {
            document.getElementById('manual-control').style.display = 'block';
        } 
        else 
        {
            document.getElementById('auto-control').style.display = 'block';
        }
    });
});

// Обработчики активности ползунка
const slider = document.getElementById('manual-power');
// Обработчик: вызывается при движении ползунка
slider.addEventListener('change', (e) => 
{        
    // Вызываем только если изменение от пользователя
    if (isUserSetManualPower) 
    {
        setManualPower();
        isPauseManualPower = true;
    }
});
slider.addEventListener('input', (e) => 
{
    document.getElementById('manual-power-value').innerText = e.target.value + '%';        
    // Вызываем только если изменение от пользователя
    if (isUserSetManualPower)
        isPauseManualPower = true;
});

// Управление ТЭНом
async function setManualPower() 
{
    const power = document.getElementById('manual-power').value;
    try 
    {
        const response = await fetch(`/api/distiller/ten?power=${power}`);
        // Ничего не парсим — просто проверяем статус
        if (response.ok)
            console.log(`success /api/distiller/ten?power=${power}`);
        else
            console.warn(`Error /api/distiller/ten?power=${power}:`, response.status);
    } 
    catch (error) 
    {
        console.error('Ошибка сети:', error);
    }
}

function setAutoStage() 
{
    const stage = document.getElementById('auto-stage').value;
    fetch(`/api/distiller/mode?stage=${stage}`)
        .then(res => res.json())
        .then(alert);
}

// Функция для программной установки значения БЕЗ вызова setManualPower
function setControl(status) 
{    
    isUserSetControl = false;
    if(!isPauseManualPower)
    {
        document.getElementById('manual-power').value = Math.round(status.ten_power);
        document.getElementById('manual-power-value').innerText = Math.round(status.ten_power) + '%';
    }
    isPauseManualPower = false;
    isUserSetControl = true;
}

//==========================================================================
// Получение текущего статуса перегонного аппарата
async function getDistillerStatus() {
    try {
        console.log("Отправка запроса: /api/distiller/status");        
        const response = await fetch('/api/distiller/status');
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        const buffer = await response.arrayBuffer();
        if (buffer.byteLength < 28) 
            {
            throw new Error(`Недостаточно данных: получено ${buffer.byteLength}, ожидается 28`);
        }
        console.log("Статус ответа:", response.status, response.statusText, "ok =", response.ok, "данные:", buffer);
        const view = new DataView(buffer);
        return {
            mode:                 view.getUint32(0,  true),   // offset 0 — целое число
            temperature_column:   view.getFloat32(4,  true),  // offset 4
            temperature_kube:     view.getFloat32(8,  true),  // offset 8
            temperature_radiator: view.getFloat32(12, true),  // offset 12
            ten_power:            view.getFloat32(16, true),  // offset 16
            voltage_220V:         view.getFloat32(20, true),  // offset 20
            fan:                  view.getUint32(24, true)    // offset 24
        };
    } catch (error) {
        console.error('Ошибка получения статуса:', error);
        return null;
    }
}

// При загрузке страницы
document.addEventListener('DOMContentLoaded', function() 
{
    updateDistillerStatus();    
    setInterval(updateDistillerStatus, 1000);
});

// Обновить отображение статуса перегонного аппарата
async function updateDistillerStatus() 
{   
    const panel = document.getElementById('status-section');
    if(panel.style.display == 'none') return;

    const status = await getDistillerStatus();
    if (!status) return; // ошибка запроса

    document.getElementById('temp-cube').textContent = status.temperature_kube.toFixed(2);
    document.getElementById('temp-column').textContent = status.temperature_column.toFixed(2);
    document.getElementById('ten-power').textContent = status.ten_power.toFixed(2);
    document.getElementById('fan-speed-percent').textContent = status.fan;
    document.getElementById('radiator-temperature').textContent = status.temperature_radiator.toFixed(0);
    setControl(status);
}

//==========================================================================
// Статистика

// Глобальные переменные для графика
let statisticData = [];       // массив полученных элементов
let lastReceivedIndex = 0;    // индекс последнего элемента на клиенте
let updatingStatistic = false; // флаг, что обновление идет

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
                data: [],
                pointHoverRadius: 2,
                pointRadius: 0,
            },
            {
                label: 'Температура колонны',
                borderColor: 'blue',
                fill: false,
                data: [],
                pointHoverRadius: 2,
                pointRadius: 0,
            },
            {
                label: 'Мощность ТЭНа',
                borderColor: 'green',
                fill: false,
                data: [],
                pointHoverRadius: 2,
                pointRadius: 0,
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

function formatTimeFromMs(ms) {
    const totalSeconds = Math.floor(ms / 1000);
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;

    // Добавляем ведущие нули
    const hStr = hours.toString().padStart(2, '0');
    const mStr = minutes.toString().padStart(2, '0');
    const sStr = seconds.toString().padStart(2, '0');

    return `${hStr}:${mStr}:${sStr}`;
}

function appendStatisticToChart(item) 
{
    // Преобразуем timestamp в читаемое время
    const time = formatTimeFromMs(item.time);
    
    // console.log(`Заносим в график time ${time} kube ${item.kube} column ${item.column} heater ${item.heater}`);

    // Добавляем метку времени
    chart.data.labels.push(time);

    // Добавляем значения в datasets
    chart.data.datasets[0].data.push(item.kube);      // Температура куба
    chart.data.datasets[1].data.push(item.column);    // Температура колонны
    chart.data.datasets[2].data.push(item.heater);    // Мощность ТЭНа

    // Ограничим длину графика (например, последние 300 точек)
    if (chart.data.labels.length > 300) {
        chart.data.labels.shift();
        chart.data.datasets.forEach(ds => ds.data.shift());
    }

    // Обновляем график только если панель видима
    const panel = document.getElementById('statistic-section');
    if (panel && panel.style.display !== 'none') {
        chart.update();
    }
}

async function lockStatisticBuffer() 
{
    try 
    {
        console.log("Отправка запроса: /api/statistic/lock");
        const resp = await fetch('/api/statistic/lock');
        console.log("Статус ответа:", resp.status, resp.statusText, "ok =", resp.ok);
        return resp.ok;
    }
    catch (e) 
    {
        console.error("Ошибка при запросе /api/statistic/lock:", e);
        return false;
    }
}

async function unlockStatisticBuffer() 
{
    try 
    {
        console.log("Отправка запроса: /api/statistic/unlock");

        const resp = await fetch('/api/statistic/unlock');

        console.log(
            "Статус ответа:",
            resp.status,
            resp.statusText,
            "ok =",
            resp.ok
        );

        return resp.ok;
    }
    catch (e) 
    {
        console.error("Ошибка при запросе /api/statistic/unlock:", e);
        return false;
    }
}

/**
 * Получить статус статистического буфера
 * @returns {Promise<{first_index: number, latest_index: number, size_available_data: number} | false>}
 */
async function getStatisticBufferStatus() 
{
    try 
    {
        console.log("Запрос статуса буфера: /api/statistic/status");
        const resp = await fetch('/api/statistic/status');

        if (!resp.ok) return false;

        const json = await resp.json();

        // читаем нужные поля
        const first_index = Number(json.firtst_index ?? json.first_index);
        const latest_index = Number(json.latest_index);
        const size_available_data = Number(json.size_available_data);

        if (isNaN(first_index) || isNaN(latest_index) || isNaN(size_available_data)) 
        {
            console.warn("Некорректные данные в ответе:", json);
            return false;
        }

        console.log("/api/statistic/status: ", json);
        return { first_index, latest_index, size_available_data };
    } 
    catch (e) 
    {
        console.error("Ошибка при запросе /api/statistic/status:", e);
        return false;
    }
}

/**
 * Получить статистику с ESP32 в бинарном формате
 * @param {number} first_index - первый индекс (включительно)
 * @param {number} latest_index - последний индекс (не включительно)
 * @returns {Promise<Array<Object>|false>} массив объектов { time, heater, column, kube } или false при ошибке
 */
async function getStatisticData(first_index, latest_index) 
{
    try 
    {
        const url = `/api/statistic/?first_index=${first_index}&latest_index=${latest_index}`;
        console.log("Запрос бинарной статистики:", url);

        const resp = await fetch(url);
        console.log("Статус ответа:", resp.status);

        if (!resp.ok) 
        {
            console.error("HTTP ошибка:", resp.status, resp.statusText);
            return false;
        }

        const arrayBuffer = await resp.arrayBuffer();
        const byteLength = arrayBuffer.byteLength; // ← КЛЮЧЕВАЯ СТРОКА

        if (byteLength % 13 !== 0) 
        {
            console.error("Некорректная длина данных: не кратна 13 байтам", byteLength);
            return false;
        }

        const dataView = new DataView(arrayBuffer);
        const count = byteLength / 13;
        const result = [];

        for (let i = 0; i < count; i++) 
        {
            const offset = i * 13;

            const time = dataView.getUint32(offset + 0, true);
            const heater = dataView.getUint8(offset + 4);
            const column = dataView.getFloat32(offset + 5, true);
            const kube = dataView.getFloat32(offset + 9, true);

            // И сохраняйте в результат
            result.push({ time, heater, column, kube });
        }

        console.log(`Получено ${result.length} точек`);
        return result;
    } 
    catch (e) 
    {
        console.error("Ошибка при запросе бинарной статистики:", e);
        return false;
    }
}

async function updateStatisticSection() 
{
    if (updatingStatistic) return; // предыдущий вызов не завершён
    updatingStatistic = true;

    const panel = document.getElementById('statistic-section');
    if (!panel || panel.offsetParent === null) 
    {
        updatingStatistic = false;
        return;
    }

    // Блокируем
    locked = await lockStatisticBuffer();
        if (!locked) return;

    // Получаем текущий статус буфера
    const status = await getStatisticBufferStatus();
    if (!status) 
    {
        await unlockStatisticBuffer();
        locked = false;
        return;
    }
    const { first_index, latest_index } = status;

    if(lastReceivedIndex < first_index)
        lastReceivedIndex = first_index;

    if (lastReceivedIndex < latest_index)
    {
        // Получаем все доступные данные
        const allData = await getStatisticData(lastReceivedIndex, latest_index);
        lastReceivedIndex = latest_index;

        if (allData && allData.length > 0) 
        {
            statisticData.push(...allData);
            lastReceivedIndex = latest_index;
            allData.forEach(item => appendStatisticToChart(item));
        }
    }
    else 
    {
        await unlockStatisticBuffer();
        locked = false;
    }

    updatingStatistic = false;
    
}

document.addEventListener('DOMContentLoaded', function() 
{
    updateStatisticSection();    
    setInterval(updateStatisticSection, 1500);
});

//==========================================================================
// Функция для переключения между секциями
function switchSections(current = "control") {
    console.log('switchSections:', current);

    const statusSection = document.getElementById('status-section');
    const controlSection = document.getElementById('control-section');
    const statisticSection = document.getElementById('statistic-section');
    const tempSensorSection = document.getElementById('temp-sensor-settings');

    statusSection.style.display = 'none';
    controlSection.style.display = 'none';
    statisticSection.style.display = 'none';
    tempSensorSection.style.display = 'none';
    
    if (current == "control") {
        statusSection.style.display = 'block';
        controlSection.style.display = 'block';
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
// Настройки датчиков температуры
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

// Обновить ROM датчика температуры
async function updateTempSensorROM() {
    const panel = document.getElementById('temp-sensor-settings');
    if (!panel || panel.offsetParent === null) return; // элемент реально скрыт
    

    const rom = await getTempROM();
    console.log('Обновление ROM датчиков температуры:', rom);
    if (!rom) return; // ошибка запроса

    document.getElementById('temp-sensor-kube-rom').innerText = rom.kube;
    document.getElementById('temp-sensor-column-rom').innerText = rom.column;
}
//==========================================================================
//Калибровка 220 В