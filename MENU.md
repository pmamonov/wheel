## Меню счетчика оборотов

```
VIEW DATA [6]	<->	2000-01-01 00:00 [1]	<->	PT 12/992: 42 [3]

^			^				^
|			|				|
v			v				v

2000-01-01 00:00 [7]	YEAR: 2000 [2]			PERIOD: 1 min [4]

^			^				^
|			|				|
v			v				v

PERIOD: 1 min [8]	MONTH: 1 [2]			DELETE OLD DATA? [5]

^			^
|			|
v			v

NP: 5 x 1B /5 [9]	DAY: 1 [2]

^			^
|			|
v			v

PT 3: 12 [10]		HOUR: 0 [2]

^			^
|			|
v			v

DOWNLOAD DATA [11]	MINUTE: 1 [2]
```

1. Текущие дата и время в формате `ГГГГ-ММ-ДД ЧЧ:ММ`.
2. Установка даты и времени. Кнопки **ВЛЕВО**/**ВПРАВО** уменьшают/увеличивают 
значение.
3. Текущее значение счетчика оборотов за период (`42`), а также количество 
периодов сохраненных в памяти прибора / емкость памяти прибора\* (`12/992`).
Если накопление данных не было начато, то отображается `PT -1/-1: 0`.
4. Установка периода подсчета оборотов. Кнопки **ВЛЕВО**/**ВПРАВО** 
уменьшают/увеличивают значение.
5. Запуск накопления данных. Нажатие **ВНИЗ** запускает новое накопление 
данных.  **РЕЗУЛЬТАТЫ ПРЕДЫДУЩЕГО НАКОПЛЕНИЯ БУДУТ ПОТЕРЯНЫ!**
6. Просмотр сохраненных результатов накопления данных.
7. Дата начала накопления.
8. Период накопления данных.
9. Количество периодов накопленных в памяти прибора (`5`) и количество байт
данных используемых для хранения оборотов за период (1В). \*
10. Количество оборотов (`12`) за период (`3`). Кнопки **ВЛЕВО**/**ВПРАВО** 
уменьшают/увеличивают номер отображаемого периода.
11. Подключите прибор к ПК с помощью переходника USB-UART. На ПК запустите 
команду `python wheel-dl.py COM_PORT file.txt` ([скачать 
wheel-dl.py](https://github.com/pmamonov/wheel/raw/ref/wheel-dl.py), требуется 
[установить python](https://www.python.org/downloads/windows/) и 
[pyserial](https://pyserial.readthedocs.io/en/latest/pyserial.html#installation)).
В команде необходимо заменить `COM_PORT` на название COM порта к которому 
подключен прибор, например, `COM3` в Windows либо `/dev/ttyUSB0` в Linux. После 
запуска команды нажмите кнопку прибора **ВПРАВО**.  Сохраненные в приборе 
данные будут переданы в ПК и записаны в файл `file.txt`.

\* Количество байт используемых для хранения оборотов за период и емкость 
памяти прибора (в периодах) зависят от длительности периода.
