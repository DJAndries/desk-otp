SRC = $(wildcard src/*.c)
SRC_OBJ = $(SRC:src/%.c=obj/%.o)

all: obj bin bin/desk-otp bin/libdotp.so bin/libfbmagic.so

FADD_FLAGS = $(ADD_FLAGS) -Wl,-rpath='$$ORIGIN'

bin/desk-otp: $(SRC_OBJ)
	gcc $(FADD_FLAGS) -lcrypto -lssl -lgpiod -ldotp -lfbmagic -L ../fbmagic/lib -L ../dotp/lib -o $@ $(SRC_OBJ)

bin/libdotp.so: bin
	cp ../dotp/lib/libdotp.so ./bin

bin/libfbmagic.so: bin
	cp ../fbmagic/lib/libfbmagic.so ./bin

obj/%.o: src/%.c
	gcc $(FADD_FLAGS) -I ../dotp/include -I ../fbmagic/include -Wall -O -c $< -o $@

obj:
	mkdir obj

bin:
	mkdir bin

clean:
	rm -f obj/* bin/*

install: bin/desk-otp
	useradd -G video,gpio -M -s /usr/bin/nologin deskotp || true
	usermod -L deskotp
	install -m 755 -d /usr/local/share/desk-otp/res
	install -m 644 ./res/* /usr/local/share/desk-otp/res/
	install -m 755 ./bin/desk-otp /usr/local/bin/
	install -m 644 ./bin/libfbmagic.so /usr/lib/
	install -m 644 ./bin/libdotp.so /usr/lib/
	install -m 644 ./src/desk-otp.service /etc/systemd/system
	touch /etc/desk-otp-keys.conf
	chmod 660 /etc/desk-otp-keys.conf
	chgrp deskotp /etc/desk-otp-keys.conf
	systemctl enable desk-otp
	systemctl start desk-otp
