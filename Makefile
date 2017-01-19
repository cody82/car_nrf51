all:
	cd car_app/pca10028/s130/armgcc && make && make genpkg
	cd car_remote/pca10028/s130/armgcc && make

remote:
	cd car_remote/pca10028/s130/armgcc && make

app:
	cd car_app/pca10028/s130/armgcc && make && make genpkg
	