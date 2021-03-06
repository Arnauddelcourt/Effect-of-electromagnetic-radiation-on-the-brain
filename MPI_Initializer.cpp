#include "MPI_Initializer.h"
#include <iostream>

// Constructor:
MPI_Initializer::MPI_Initializer(int argc, char *argv[],int required){
	this->required = required;
	#if DEBUG > 1
	cout << "MPI_Initializer::constructor::IN" << endl;
	#endif
	
	/* MPI initialization: */
	int provided = INT_MIN;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &(provided) );
	this->provided = provided;
	
	// MPI Query: check that the required level of thread support is effectively the 
	// provided level of thread support:
	if(this->provided.get() != this->required.get()){
		cout << "MPI_Initializer::constructor::\n\tRequired level of thread support is ";
		cout << this->required.get() << " and the provided level of thread support is ";
		cout << this->provided.get() << ".\n";
		// If the required level of thread support is higher than the provided one, abort:
		if(this->required.get() > this->provided.get()){
			// The current machine cannot provided the required level of
			// thread support. Ask what to do.
			cout << "The required level of thread support is higher than the provided one.\n";
			cout << "For the communication between the MPI processes to work flowlessly";
			cout << ", MPI_THREAD_MULTIPLE (or at least MPI_THREAD_SERIALIZED).\n";
			cout << "You have thread support ";
			if(this->provided.get() == MPI_THREAD_SINGLE ){
				cout << "MPI_THREAD_SINGLE. Aborting because it is too low.\n";
				abort();
			}else if(this->provided.get() == MPI_THREAD_FUNNELED ){
				cout << "MPI_THREAD_FUNNELED. Aborting because it is too low.\n";
				abort();
			}else if(this->provided.get() == MPI_THREAD_SERIALIZED ){
				cout << "MPI_THREAD_SERIALIZED. The communications will be severely affected";
				cout << " but it is OK.\n";
			}else if(this->required.get() == MPI_THREAD_MULTIPLE ){
				cout << "MPI_THREAD_MULTIPLE. This is the best thread support you can have.";
				cout << " We will intensively rely on that to communicate.\n";
			}
		}
	}
	
	// Get the ID of the MPI process:
	int ID_MPI_Process = INT_MIN;
	MPI_Comm_rank( MPI_COMM_WORLD, &ID_MPI_Process);
	this->ID_MPI_Process = ID_MPI_Process;
	
	// Get the number of MPI processes:
	int number_of_MPI_Processes = INT_MIN;
	MPI_Comm_size( MPI_COMM_WORLD, &number_of_MPI_Processes);
	this->number_of_MPI_Processes = number_of_MPI_Processes;
	cout << "number of MPI processes is " << this->number_of_MPI_Processes.get() << endl;
	#if DEBUG > 1
	cout << "MPI_Initializer::constructor::OUT" << endl;
	#endif
}

// Destructor:
MPI_Initializer::~MPI_Initializer(void){
	#if DEBUG > 1
	cout << "MPI_Initializer::destructor::IN" << endl;
	#endif
	MPI_Finalize();
	#if DEBUG > 1
	cout << "MPI_Initializer::destructor::OUT" << endl;
	#endif
}

// Is this MPI process the root one?
// If the process is not the root one, than send INT_MIN.
int MPI_Initializer::isRootProcess(void){
	if(this->ID_MPI_Process.get() == ROOT_PROCESSOR){
		return this->ID_MPI_Process.get();
	}else{
		return INT_MIN;
	}
}

// Get rank/ID of the MPI process:
int MPI_Initializer::getRank(void){
	return this->ID_MPI_Process.get();
}


void MPI_Initializer::MpiDivision(GridCreator &subGrid){  

	int nbProc = this->number_of_MPI_Processes.get();
	int myRank = this->ID_MPI_Process.get();

	double Lx = subGrid.input_parser.lengthX;
	double Ly = subGrid.input_parser.lengthY;
	double Lz = subGrid.input_parser.lengthZ;

	double deltaX = subGrid.input_parser.deltaX;
	double deltaY = subGrid.input_parser.deltaY;
	double deltaZ = subGrid.input_parser.deltaZ;


	int N = (int) pow(nbProc, 1.0/3.0);
	std::vector<double> mpiExtremity;

	/* RankNeighbour[0] = SOUTH (along the opposite direction of the x-axis)
	RankNeighbour[1] = NORTH (along the direction of the x-axis)
	RankNeighbour[2] = WEST (along the opposite direction of the y-axis)
	RankNeighbour[3] = EAST (along the direction of the y-axis)
	RankNeighbour[4] = DOWN (along the opposite direction of the z-axis)
	RankNeighbour[5] = UP (along the direction of the z-axis) */
	int RankNeighbour[6];

	int i;


	// Cubic case
	if(N*N*N == nbProc){
		double LxLocal = Lx/N;
		double LyLocal = Ly/N;
		double LzLocal = Lz/N;
		mpiExtremity.push_back((myRank%N)*LxLocal);
		mpiExtremity.push_back(((myRank%N)+1)*LxLocal);
		mpiExtremity.push_back( (((int)(myRank/N) - (int) (myRank/(N*N))*N )) *LyLocal);
		mpiExtremity.push_back((((int)(myRank/N) - ((int) (myRank/(N*N) ))*N )+1)*LyLocal);
		mpiExtremity.push_back( myRank/ (N*N) * LzLocal);
		mpiExtremity.push_back(((myRank/(N*N))+1)*LzLocal);

		/* myRank%N gives the current position on the x-axis */
		/* (((int)(myRank/N) - (int) (myRank/(N*N))*N )) gives the current position on the y-axis */
		/* myRank/ (N*N) gives the current position on the z-axis */

		int PositionOnX = myRank%N;
		int PositionOnY = (((int)(myRank/N) - (int) (myRank/(N*N))*N ));
		int PositionOnZ = myRank/ (N*N);

		if(N == 1)
		{
			this->RankNeighbour[0] = -1;
			this->RankNeighbour[1] = -1;
			this->RankNeighbour[2] = -1;
			this->RankNeighbour[3] = -1;
			this->RankNeighbour[4] = -1;
			this->RankNeighbour[5] = -1;
		}
		else
		{
		/* --------------------------------------------------------------------------------------------------- */
		/* We do the x component */
			if(PositionOnX == 0)
			{
				this->RankNeighbour[0] = -1;
				this->RankNeighbour[1] = myRank+1;
			}
			else if(PositionOnX == N-1)
			{
				this->RankNeighbour[0] = myRank-1;
				this->RankNeighbour[1] = -1;
			}
			else
			{
				this->RankNeighbour[0] = myRank-1;
				this->RankNeighbour[1] = myRank+1;
			}

			/* We do the y component */
			if(PositionOnY == 0)
			{
				this->RankNeighbour[2] = -1;
				this->RankNeighbour[3] = myRank+N; 
			}
			else if(PositionOnY == N-1)
			{
				this->RankNeighbour[2] = myRank-N;
				this->RankNeighbour[3] = -1;
			}
			else
			{
				this->RankNeighbour[2] = myRank-N;
				this->RankNeighbour[3] = myRank+N;
			}

			/* We do the z component */
			if(PositionOnZ == 0)
			{
				this->RankNeighbour[4] = -1;
				this->RankNeighbour[5] = myRank+N*N;
			}
			else if(PositionOnZ == N-1)
			{
				this->RankNeighbour[4] = myRank-N*N;
				this->RankNeighbour[5] = -1;
			}
			else
			{
				this->RankNeighbour[4] = myRank-N*N;
				this->RankNeighbour[5] = myRank+N*N;
			}
			/* --------------------------------------------------------------------------------------------------------------- */
		}
	}

	//Impair case
	else if(nbProc %2 != 0){
		double LxLocal = Lx/nbProc;
		double LyLocal = Ly;
		double LzLocal = Lz;

		// Coordinates of all subdivisions in the order -> Lx, Ly, Lz
		mpiExtremity.push_back(myRank*LxLocal);
		mpiExtremity.push_back((myRank+1)*LxLocal);
		mpiExtremity.push_back( 0);
		mpiExtremity.push_back(LyLocal);
		mpiExtremity.push_back(0);
		mpiExtremity.push_back(LzLocal);

		int PositionOnX = myRank;
		int PositionOnY = 0;
		int PositionOnZ = 0;

		/* We do the x component */
		if(PositionOnX == 0)
		{
			this->RankNeighbour[0] = -1;
			this->RankNeighbour [1] = myRank+1;
		}
		else if(PositionOnX == nbProc-1)
		{
			this->RankNeighbour[0] = myRank-1;
			this->RankNeighbour[1] = -1;
		}
		else
		{
			this->RankNeighbour[0] = myRank-1;
			this->RankNeighbour[1] = myRank+1;
		}

		/* We put -1 everywhere because we have only a separation along the x-axis */
		this->RankNeighbour[2] = -1;
		this->RankNeighbour[3] = -1;
		this->RankNeighbour[4] = -1;
		this->RankNeighbour[5] = -1;

	}

	//Pair case
	else{
		double LxLocal = 2*Lx/nbProc;
		double LyLocal = Ly/2;
		double LzLocal = Lz;
		mpiExtremity.push_back(myRank%(nbProc/2)*LxLocal);
		mpiExtremity.push_back(((myRank%(nbProc/2))+1)*LxLocal);
		mpiExtremity.push_back(((int)(2*myRank/nbProc))*LyLocal);
		mpiExtremity.push_back(((int)((2*myRank/nbProc))+1)*LyLocal);
		mpiExtremity.push_back(0);
		mpiExtremity.push_back(LzLocal); // Coordinates of all subdivisions in the order -> Lx, Ly, Lz

		int PositionOnX = myRank%(nbProc/2);
		int PositionOnY = ((int)(2*myRank/nbProc));
		int PositionOnZ = 0;

		/* We do the x component */
		if(nbProc == 2){
			this->RankNeighbour[0] = -1;
			this->RankNeighbour[1] = -1;
			cout << myRank <<"!!!!!!!!!!!!!!!!!!!!!! NTP !!! RankNeighbour x: " <<this->RankNeighbour[0] << " ; " <<this->RankNeighbour[1] << endl;
		}
		else{
			if(PositionOnX == 0)
			{
				this->RankNeighbour[0] = -1;
				this->RankNeighbour[1] = myRank + 1;
				
				cout << myRank <<"!!!!!!!!!!!!!!!!!!!!!! NTM !!! RankNeighbour x: " <<this->RankNeighbour[0] << " ; " <<this->RankNeighbour[1] << endl;

			}
			else if(PositionOnX == nbProc/2 -1)
			{
				this->RankNeighbour[0] = myRank-1;
				this->RankNeighbour[1] = -1;
				cout << myRank <<"!!!!!!!!!!!!!!!!!!!!!!RankNeighbour x: " <<this->RankNeighbour[0] << " ; "<<this->RankNeighbour[1] << endl;	
			}
			else
			{
				this->RankNeighbour[0] = myRank-1;
				this->RankNeighbour[1] = myRank+1;
				cout << myRank<< "!!!!!!!!!!!!!!!!!!!!!!RankNeighbour x: " <<this->RankNeighbour[0] << " ; "<<this->RankNeighbour[1] << endl;
			}
		}
		/* We do the y component */
		if(PositionOnY == 0)
		{
			this->RankNeighbour[2] = -1;
			this->RankNeighbour[3] = myRank+(nbProc/2);
			cout << myRank <<"!!!!!!!!!!!!!!!!!!!!!!RankNeighbour y: " <<this->RankNeighbour[2] << " ; "<< this->RankNeighbour[3] << endl;
		}
		else if(PositionOnY == 1)
		{
			this->RankNeighbour[2] = myRank-(nbProc/2);
			this->RankNeighbour[3] = -1;
			cout << myRank <<"!!!!!!!!!!!!!!!!!!!!!!RankNeighbour y: " <<this->RankNeighbour[2] << " ; "<<this->RankNeighbour[3] << endl;
		}
		else
		{
			printf("MPI_Initializer::MpiDivision\n");
			printf("This case is not envisageable, at line %d in file %s.\n",__LINE__,__FILE__);
			abort();

		}

		/* We do the z component */
		this->RankNeighbour[4] = -1;
		this->RankNeighbour[5] = -1;
		cout << myRank<< "!!!!!!!!!!!!!!!!!!!!!!RankNeighbour z: " <<this->RankNeighbour[4] << " ; "<<this->RankNeighbour[5] << endl;

	}

    // transformation of global lengths to indices and save them in subdomain
	subGrid.originIndices.push_back(mpiExtremity[0]/deltaX);
	subGrid.originIndices.push_back(mpiExtremity[2]/deltaY);
	subGrid.originIndices.push_back(mpiExtremity[4]/deltaZ);
	cout << myRank<< "---------------------->  x: " <<subGrid.originIndices[0] << endl;
	cout << myRank<< "---------------------->  y: " <<subGrid.originIndices[1] << endl;
	cout << myRank<< "---------------------->  z: " <<subGrid.originIndices[2] << endl;

	// Length of the subdomains
	subGrid.lengthX = mpiExtremity[1]-mpiExtremity[0];
	subGrid.lengthY = mpiExtremity[3]-mpiExtremity[2];
	subGrid.lengthZ = mpiExtremity[5]-mpiExtremity[4];
	

}

bool MPI_Initializer::SendDataToNeighboor(double *vectorToSend,
										  size_t lengthToSend,
										  unsigned char direction){
	// We can communicate in 6 directions
	// By default, it is assumed that everything worked fine.
	// Thus, return true.
	return true;
}
