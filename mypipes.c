int fistPipe(int **matrix, tline* line, int nPipes){
	if(line->redirect_input != NULL){ // input redirect
		change_redirections(line, 0);
	}
				// Hay que cerrar todos los pipes que no se vayan a usar
	for (int j = 1; j < nPipes; ++j){
		close(pipes_matrix[j][0]);
		close(pipes_matrix[j][1]);
	} 
	close(pipes_matrix[0][0]); // Cerramos stdin del pipe que vamos a usar
	dup2(pipes_matrix[0][1],STDOUT_FILENO);
	close(pipes_matrix[0][1]);
	return 0;
}


int lastPipe(int **matrix, tline* line, int nPipes);
int mediumPipe(int **matrix, tline* line, int nPipes);