
#include <iostream>
#include <opencv2/opencv.hpp>
#include <./common/kb_searchfiles.h>
#include <./common/kb_csv.h>

int main(int argc, char* argv[])
{
  
//    cv::Mat mat1 = cv::imread(argv[1]);

//    cv::namedWindow("image", cv::WINDOW_NORMAL);
//    cv::imshow("image", mat1);
//    cv::waitKey(0);

  std::string dname_in=argv[2];
  std::string search_key=argv[3];
  
 	//	input image list
	std::vector< std::string > filenames;
	if (kb::search_files(dname_in, search_key.c_str(), filenames) < 0)
		return -1;

	int	num_files = filenames.size();
	std::cout << "number of files = " << num_files << std::endl;
	if (num_files < 2)
		return -1;
  for(int k=0;k<num_files;k++){
    std::cout<<filenames[k]<<std::endl;
  }

    return 0;
}

