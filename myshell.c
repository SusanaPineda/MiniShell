#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>

//Ejecucion en shell: gcc test.c -o nombrearchivo -l:libparser_64.a
#include "parser.h"


void ejecutar1(tline *line);
void ejecutar2(tline *line);
void ejecutarn(tline *line);
void gestionCd(tline *line, char *cwd);
void gestionRedireccionEntrada(tline *line);
void gestionRedireccionSalida(tline *line);



int main(void){
	//Indicamos a la minishell que ignore las señeles sigInt y sigQuit
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);


	//DECLARACION DE VARIABLES
	int i,j;
	char buf[1024];
	tline * line;
	char cwd[1024];
	
	
	printf("%s $ ", getcwd(cwd, sizeof(cwd)));	//Prompt

	while (fgets(buf,1024, stdin)) { //Lectura de Mandatos
		line = tokenize(buf); // Conversion datos leidos a estructura.
		
		//Casos Posibles
		//linea sin nada:
		if(sizeof(line)==0){
			continue; //No hacemos nada, mostramos el prompt de nuevo
		}
		//Si la linea tiene UN unico mandatos.
		if(line->ncommands==1){ 
			//Comprobamos si el mandato es cd, si no ejecutamos normal:
			if( strcmp(line->commands[0].argv[0], "cd") ==0 ){
	  			gestionCd(line, cwd);
	  		}

	  		else{
	  			ejecutar1(line);
	  		}
		}
		//Si la linea tiene dos mandatos.
		if(line->ncommands == 2){ 
			ejecutar2(line);
		}
		//Si la linea tiene mas de dos mandatos
		if(line->ncommands > 2){ 
			ejecutarn(line);
		}
		printf("%s $ ", getcwd(cwd, sizeof(cwd)));	//Prompt
	}
	
	return 0;
}


void ejecutar1(tline *line){  

		int pid=fork();
		int status;
		
		//En el caso de que se reciba la señal SIGINT o SIGQUIT en este proceso si que se le
		//hace caso.
		signal(SIGINT,SIG_DFL);
		signal(SIGQUIT,SIG_DFL);

		if (pid < 0) { // Error 
			fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
			exit(1);
		}

		if (pid == 0) { // Proceso Hijo
			//Gestion de redirecciones:
			gestionRedireccionEntrada(line);
			gestionRedireccionSalida(line);

			//Ejecucion del mandato
			execvp(line->commands[0].filename, line->commands[0].argv);
			//Si llega aquí es que se ha producido un error en el execvp
			printf("Error al ejecutar el comando: %s\n", strerror(errno));
						
		}
			
		else { // Proceso Padre. 
			wait (&status);
			if (WIFEXITED(status) != 0)
				if (WEXITSTATUS(status) != 0)
					printf("Comando no ejecutado correctamente\n");
		}
}

		/* Si hay redireccion de entrada o de salida se comprueba si existe el archivo
		se abre el archivo y se redirecciona la entrada o la salida mediante dup2()
		-- dup2(fd1,1): Redirecciona la salida estandar para que se escriba en fd1
		-- dup2(fd1,0): Redirecciona la entrada estandar para que sea fd1.
		*/

void gestionRedireccionEntrada(tline *line){
	int fdE; //Descriptor de fichero de redireccion de entrada redirect_input

	//Si hay redireccion de entrada:
	if (line->redirect_input != NULL) {
		/*abrimos el archivo de redireccion indicado con redirect_input
		int open(char *name, int flags[,mode_t mode]);
		name: cadena de caracteres con el nombre del fichero
		flags: 	O_RDONLY el fichero se abre solo de lectura
				O_WDONLY el fichero se abre solo de escritura
				O_CREAT si no existe el fichero secrea y no da error
		*/
		fdE = open(line->redirect_input,O_RDONLY);
					
		//Comprobacion de errores redireccion de entrada:
		if (fdE != -1){
		//cambiamos la entrada estandar por el fichero si no hay error
			dup2(fdE,0);
		}

		else {
			printf("El archivo no se ha podido abrir");
		}
					
		//cerramos el fichero una vez que hemos terminado
		fdE = close (fdE);
		if (fdE = -1){
			printf ("El archivo no se ha cerrado correctamente");
		}
	}

	//Comprobacion de errores de redireccion:
	if (line->redirect_error != NULL) {
		printf("Error en la redireccion: %s\n", line->redirect_error);
	}
	
}

void gestionRedireccionSalida(tline *line){
	int fdS; //Descriptor de fichero de redireccion de salida redirect_output

	//Si hay redireccion de salida:
	if (line->redirect_output != NULL) {
		//abrimos el archivo que vamos a editar:
		fdS = open(line->redirect_output,O_WRONLY|O_CREAT,S_IRWXU);

		//Comprobacion de errores redireccion de salida
		if (fdS != -1){
		//cambiamos la entrada estandar por el fichero si no hay error
			dup2(fdS,1);
		}

		else{
			printf("El archivo no se ha podido abrir o crear el archivo");
		}
					
		//cerramos el fichero una vez que hemos terminado
		fdS = close (fdS);
		if (fdS = -1){
			printf ("El archivo no se ha cerrado correctamente");
		}
	}

	//Comprobacion de errores de redireccion:
	if (line->redirect_error != NULL) {
		printf("Error en la redireccion: %s\n", line->redirect_error);
	}

}

void ejecutar2(tline *line){
	//declaracion de variables:
	pid_t pid1, pid2; //Proceso principal, proceso 1 y proceso 2
	int p[2]; //tuberia
	
	pipe(p); //creamos la tuberia 
	int status0;

	//En el caso de que se reciba la señal SIGINT o SIGQUIT en este proceso si que se le
	//hace caso.
	signal(SIGINT,SIG_DFL);
	signal(SIGQUIT,SIG_DFL);

		pid1 = fork(); //Creamos  el hijo encargado de escribir en el pipe
		
		//HIJO 1 ESCRIBIR E0
		if (pid1 == 0){
			close(p[0]); //se cierra la salida del pipe
			dup2(p[1],1); //redireccionamos la salida estandar al pipe

			//Gestion de redirecciones. Solo puede ser de Entrada
			gestionRedireccionEntrada(line);

	  		//Ejecucion del mandato
			execvp(line->commands[0].filename, line->commands[0].argv);
			printf("Error al ejecutar el comando%s\n", strerror(errno));
			
	  		
		}

		pid2 = fork();//Creamos el hijo encargado de leer
		
		//HIJO 2 LEER E1
		if (pid2 == 0){
			close(p[1]); //se cierra la entrada del pipe
			dup2(p[0],0);//redireccionamos la entrada estandar al pipe

			//Gestion de redirecciones. Solo puede ser de Salida
			gestionRedireccionSalida(line);
			
			//Ejecucion del mandato
			execvp(line->commands[1].filename, line->commands[1].argv);
			printf("Error al ejecutar el comando%s\n", strerror(errno));
			
	  		
		}
	
	else{
		/*El padre espera a que terminen los procesos y cierra el pipe.*/
		wait(NULL);
		close(p[0]);
		close(p[1]);
		wait (&status0);
		if (WIFEXITED(status0) != 0)
			if (WEXITSTATUS(status0) != 0)
				printf("comando no ejecutado correctamente\n");
	}

}


void ejecutarn(tline *line){
//Declaracion de variables:
	int i;
	int p[2];
	int nuevop[2];
	int status1;

	pid_t pid;

	pipe(p);//creamos la tuberia principal

	//En el caso de que se reciba la señal SIGINT o SIGQUIT en este proceso si que se le
	//hace caso.
	signal(SIGINT,SIG_DFL);
	signal(SIGQUIT,SIG_DFL);

	for(i=0; i<line->ncommands; i++){

		if(i>0 && i<line->ncommands-1){ 
		//Si es un mandato intermedio hacemos un pipe para poder manejar la informacion correctamente.
			pipe(nuevop);
		}

		pid = fork(); //Creamos los hijos.

		if(pid<0){
			printf("Error al crear el proceso\n");
		}

		if(pid == 0){ //Estamos en algun hijo.

			if(i==0){ //Primer mandato.
				//Comprobamos si hay redireccion de entrada:
				gestionRedireccionEntrada(line);
				
				close(p[0]); //cerramos la salida del pipe
				dup2(p[1],1); //redireccionamos la salida estandar a la entrada del pipe
				close(p[1]); //cerramos la entrada del pipe

	  			execvp(line->commands[i].argv[0], line->commands[i].argv); //Ejecutamos.
	  			printf("Error al ejecutar el comando%s\n", strerror(errno));
				exit(1);
	  			
			}

			else if(i>0 && i<line->ncommands-1){ //Mandatos intermedios

				dup2(p[0],0); //Redireccionar entrada estandar a salida del pipe
				close(p[0]); //cerramos el pipe p
				close(p[1]);

				close(nuevop[0]);  //cerramos la salida del pipe auxiliar
				dup2(nuevop[1],1); //Redireccionar salida del pipe duplicado a entrada estandar
				close(nuevop[1]); //cerramos la entrada del pipe auxiliar.

	  			execvp(line->commands[i].argv[0], line->commands[i].argv); //Ejecutamos.
	  			printf("Error al ejecutar el comando%s\n", strerror(errno));
				exit(1);
	  			

			}

			else if(i == line->ncommands-1){ //Ultimo mandato

				dup2(p[0],0); //Redireccionamos la entrada estandar a la salida del pipe;
				
				//comprobamos si hay redireccion de slida:
				gestionRedireccionSalida(line);

				close(p[0]); //cerramos el pipe principal
				close(p[1]);

	  			execvp(line->commands[i].argv[0], line->commands[i].argv); //Ejecutamos.
	  			printf("Error al ejecutar el comando%s\n", strerror(errno));
				exit(1);
	  			
			}

		} //Fin if hijos

		else{ //Ejecucion del padre...

			if(i!=0){ 
				//Si no es el primer mandato cerramos el pipe principal
				close(p[0]); 
				close(p[1]);
			}
			if(i>0 && i<line->ncommands-1){ //Si no es el ultimo lo copiamos para encadenarlo con el siguiente.
				p[0] = nuevop[0];
				p[1] = nuevop[1];
			}
		}

	}//Fin For

	int k;
	for(k=0; k<line->ncommands; k++){ //Esperar a que los hijos terminen para volver a mostrar el prompt
		wait(NULL);
	}

	//Cerramos los pipes
	close(p[0]);
	close(p[1]);
	close(nuevop[0]);
	close(nuevop[1]);

	//esperamos a estatus y continuamos:
	wait (&status1);
		if (WIFEXITED(status1) != 0)
			if (WEXITSTATUS(status1) != 0)
				printf("comando no ejecutado correctamente\n");
}

void gestionCd(tline *line, char *cwd){
	//En el caso de que se reciba la señal SIGINT o SIGQUIT en este proceso si que se le
	//hace caso.
	signal(SIGINT,SIG_DFL);
	signal(SIGQUIT,SIG_DFL);
	
	//Casos Posibles
	//Si es solo cd vamos al HOME: 
	if(line->commands[0].argv[1] == NULL){
		chdir(getenv("HOME"));
	}
	//Si es cd x :
	else if(line->commands[0].argc > 1){
		//Si x == ".." vamos al padre:
		if( strcmp(line->commands[0].argv[1], "..") == 0){
			//printf("PWD = %s\n", dirname(getenv()));
			chdir(dirname(cwd));

		}
		//Si x es una ruta absoluta: /home/susana o relativa
		else{
			chdir(line->commands[0].argv[1]);
		}
		
	}

}