#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include <cstdlib>
#include <assert.h>

// #define DEBUG 1

#ifdef DEBUG
#define Debug(stuff) std::cout << (stuff);
#else
#define Debug(stuff)
#endif

extern "C" {
#include <roi.h>
}
using namespace std;


void print_arr( int **arr, const int SIZE)
{
  int row = 0, col=0;

  for(row=0;row<SIZE;row++)
  {
    for(col=0;col<SIZE;col++)
      cout<<arr[row][col]<<" ";
    cout<<endl;
  }


}


bool readsudoku(const char* input_file, int** &arr,const int SIZE)
{
  ifstream inFile(input_file);
  // ifstream inFile("input_sudoku.txt");
  if(!inFile.is_open())
  {
    cerr<<"Could not open file \n";
    return false;
  }

  int row=0, col=0;

  //Initialize arrays
  //Initialize rows
  arr = new int* [SIZE];
  //initialize columns
  for (int i=0; i<SIZE;i++)
  {
    arr[i] = new int [SIZE];
  }


  string s;
  while( getline(inFile,s))
  {

    int value;
    stringstream ss;
    ss<<s;

    while(ss>>value)
    {
      assert(row<SIZE && col < SIZE && "Either # rows or # cols are greater than SIZE, PLEASE SET CORRECT SIZE ");
      arr[row][col] = value;
      col++;
    }

    assert(col == SIZE && "#col != SIZE" );

  __app_roi_begin();
    row++;
    col=0;
  __app_roi_end();

  }

  assert(row == SIZE && "#row != #SIZE" );


  return true;
}

bool find_unassigned(int** arr, const int SIZE, int & row, int & col)
{
  for(row=0;row<SIZE;row++)
  {
    for(col=0;col<SIZE;col++)
    {
      if(arr[row][col] == 0)
        return true;
    }
  }

  return false;

}

bool is_row_safe( int** arr, const int SIZE, int num, int row)
{
  int col=0;

  for(col=0; col<SIZE; col++)
    if(arr[row][col] == num)
      return false;

  return true;

  // cout<<"ROW SAFE: num: "<<num<<endl;
}

bool is_col_safe( int** arr, const int SIZE, int num, int col)
{
  int row=0;
  for(row=0; row<SIZE; row++)
    if(arr[row][col] == num)
      return false;

  return true;

  // cout<<"COL SAFE: num: "<<num<<endl;
}

bool is_box_safe( int** arr, const int SIZE, int num, int row,  int col)
{
  // cout<<"num= "<<num<<" row: "<<row<<" col: "<<col<<endl;
  int box_size=sqrt(SIZE);
  int row_start= (row/ box_size)*box_size;
  int row_end= row_start + box_size;

  int col_start= (col/ box_size)*box_size;
  int col_end= col_start + box_size;

  int i= row_start;
  int j = col_start;

  for(i=row_start; i<row_end; i++)
  {
    for(j=col_start; j<col_end; j++)
    {
      // cout<<"a["<<i<<"]["<<j<<"] ="<<arr[i][j]<<endl;
      if(arr[i][j] == num)
        return false;
    }
  }
  // cout<<"BOX SAFE: num: "<<num<<endl;
  return true;

}

bool is_safe(int** arr, const int SIZE, int num, int row, int col)
{

  if(is_box_safe(arr,SIZE, num, row, col) && is_col_safe(arr,SIZE ,num, col) && is_row_safe(arr,SIZE, num, row) )
    return true;
  else 
    return false;

}

//TODO: FIND A WAY TO FIND IF INPUT IS WRONG AND ITS NOT POSSIBLE to solve
//CAN PUT A CHECK TO SEE IF ALL GIVEN INPUTS ARE BOX SAFE, ROW SAFE AND COL SAFE
bool sudoku_solver(int** arr,const int SIZE)
{
  int row=0, col=0;

  Debug(cout<<"*********-----------------****************"<<endl);
  // find unassigned will set row and col

  if(find_unassigned(arr,SIZE, row, col))
  {
    Debug(cout<<"Unassigned row: "<<row<<" col: "<<col<<endl);
    for(int num=1; num<=SIZE;num++)
    {
      if(is_safe(arr, SIZE, num, row, col))
      {
        Debug(cout<<"SET: a["<<row<<"]["<<col<<"] = "<<num<<endl);
        arr[row][col] = num;
        if(sudoku_solver(arr, SIZE))
          return true;
        else 
        {
          //unassign if next iteration of solver
          //returns false and try next number
          Debug(cout<<"BACKTARCK: arr["<<row<<"]["<<col<<"]"<<endl);
          arr[row][col] = 0;
          continue;

        }
      }
      else 
      {
        // cout<<"NOT SAFE: num: "<<num<<endl;
        // cout<<"-----------------"<<endl;
      }
    }


    // cout<<"--Printing sudoku ---------------"<<endl;
    // print_arr(arr,SIZE);

    return false;
  }
  else 
  {
    cout<<"solved Sudoku"<<endl;
    print_arr(arr, SIZE);
    return true;

  }




}

int main(int argc, char** argv)
{

  cout<<"argc: "<<argc<<endl;
  if(argc != 3) assert(0 && "Please provide 2 arguments, arg[0]= SIZE, arg[1] = input_file (space separated)");
  cout<<"SIZE of sudoku: "<<argv[1]<<endl;
  cout<<"Input file: "<<argv[2]<<endl;
  const int SIZE = atoi(argv[1]);


  int box_size = sqrt(SIZE);
  float box_size2 = sqrt(SIZE);
  assert(box_size == box_size2 && "SIZE of SUDOKU is not a perfect square");


  //Read SUDOKU FILE
  // int arr[SIZE][SIZE];
  // int* arr = new int[SIZE];
  // const char* input_file = "input_sudoku.txt";
  int **arr = NULL;
  const char* input_file = argv[2]; 
  readsudoku(input_file, arr, SIZE);


  //Print SUDOKU FILE
  cout<<"Initial Sudoku \n";
  print_arr(arr, SIZE);


  sudoku_solver(arr, SIZE);



  return 0;
}
