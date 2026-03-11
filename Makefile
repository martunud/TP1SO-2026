# Abre el contenedor de docker dado por la cátedra
DOCKER_IMAGE = agodio/itba-so-multiarch:3.1
# Para ejecutar el contenedor, se debe correr el comando "make dockerarm" o "make dockeramd" dependiendo de la arquitectura que se quiera usar. 
# Esto montará el directorio RunARM o RunAMD respectivamente, y se abrirá una terminal dentro del contenedor con ese directorio como raíz. 
# Desde ahí, se pueden ejecutar los comandos necesarios para compilar y correr el código dentro del contenedor.
dockerarm:
	docker run -v "$(shell pwd)/RunARM:/root" -w /root --privileged -ti $(DOCKER_IMAGE)

dockeramd:
	docker run -v "$(shell pwd)/RunAMD:/root" -w /root --privileged -ti $(DOCKER_IMAGE)