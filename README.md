Простой пример _своего_ кода для контроллера Segnetics SMH4.

Работает с программным ядром контроллера и через shared memory достает оттуда значения переменных. Работа со списком переменных не через load_files.srv, а через sqllite. Работает debug (за что и боролся с VScode). 

Чтобы доставать из ядра свои переменные (Mem в SMLogix) нужно:
1. Заполнить именами интересующих переменных массив Vars (как это сделано в main.c).
2. Добавить идентификаторы соответстующие переменным в enum MEMORY_VARS в файле vardefs.h . Это нужно для обращения к переменной как Vars[MY_SEC].value . Если такой адресацией не пользоваться, то на этот enum можно забить.

Значения переменных обновляются отдельным thread'ом с регулярностью заданной в vars_checker (в данном случае - 10 ms). Если в элементе Vars[i] определен .callback , то при изменении значения переменной он будет дергаться этим thread'ом.

Сделано на базе примеров, которые поставляются Segnetics, но без плюсов (крестов), т.к. по натуре ленив и не сподобился их освоить. Код абсолютно не вылизывался. Собирается под linux ( Ubuntu 18.04.1 ) toolchain'ом от linaro. Можно через Makefile, можно в VSCode. Конфигурацию под windows не делал.

Нормальная процедура сборки в VSCode:
1. Ctrl+Shift+B "Build" - собственоо сборка
2. Ctrl+Shift+B "install" - копирует в домашний каталог рута на контроллере. Предполагается, что комп и контроллер сидят в одной сети. Для вящего удобства стоит прогрузить свой ssh-public key в ~root/.ssh/authorized_keys на контроллере.
3. Ctrl+Shift+F7 "gdbserver Start" - запуск gdbserver на контроллере. Вместо этого можно сидя ssh на контроллере:  набирать: gdbserver :1234 ./test2
4. F5 - старт дебуга.
5. ... далее по мере способностей.

---------------- toolchain

Toolchain от linaro - https://www.linaro.org/downloads/  gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf . У меня лежит в /opt/linaro и, чтобы иметь возможность комбинировать версии, сделаны симлинки:
    arm-linux-gnueabihf -> gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf
    runtime -> runtime-gcc-linaro-7.5.0-2019.12-arm-linux-gnueabihf
    sysroot -> sysroot-glibc-linaro-2.25-2019.12-arm-linux-gnueabihf

В этом тулчайне нет libsqlite3. Соответственно, собрал его тем же тулчайном и положил его этажом выше проекта. При сборке (как и при использовании Makefile) не забывать переопределять свой PATH:
    export PATH=/opt/linaro/arm-linux-gnueabihf/bin:$PATH

При работе в VSCode прописать во всех трех файлах, в каталоге .vscode правильные абсолютные пути до тулчайна.

---------------- сборка sqlite

https://www.sqlite.org/download.html
tar -xzvf sqlite-autoconf-3320300.tar.gz
cd sqlite-autoconf-3320300/
./configure --host=arm-linux --prefix=/home/artp/Segnetics/libs CC=/opt/linaro/arm-linux-gnueabihf/bin/
make
ls -l .libs
make install


Здоровая критика приветствуется.

Enjoy.