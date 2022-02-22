
#include <iostream>
#include <opencv2/opencv.hpp>
#include <../common/kb_searchfiles.h>
#include <../common/kb_csv.h>

int main1(int argc, char* argv[])
{

	cv::Mat mat1 = cv::imread(argv[1]);

	cv::namedWindow("image", cv::WINDOW_NORMAL);
	cv::imshow("image", mat1);
	cv::waitKey(0);
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 8)
		return -1;

	std::string path_obj = argv[1];
	std::string path_class = argv[2];
	int threshold_d1 = atoi(argv[3]);
	std::string dname_out = argv[4];
	int threshold_num_seq = atoi(argv[5]);

	std::string dname_images = argv[6];
	std::string search_key = "*.jpg";
	int mode_save_images = atoi(argv[7]);
	int debug = 0;


	dname_out = dname_out + "\\";
	dname_images = dname_images + "\\";

	//	load class names
	std::cout << "load class names..." << std::endl;
	std::vector<std::string> v_class;
	std::map<std::string, int> lut_class;
	{
		std::vector<std::vector<std::string>>  vv_class;
		kb::load_csv(path_class, vv_class, ',');
		int num_class = vv_class.size();
		std::cout <<"num of class: "<< num_class << std::endl;
		for (int i = 0; i < num_class; i++) {
			int num=vv_class[i].size();
			if (num > 0) {
				v_class.push_back(vv_class[i][0]);
				std::cout << vv_class[i][0] << std::endl;
			}
		}
	}
	int num_class = v_class.size();
	{
		for (int i = 0; i < num_class; i++) {
			lut_class.insert(std::pair<std::string, int>(v_class[i], i));
		}
	}

	if (debug > 0) {
		getchar();
	}


	//	load detection results
	std::cout << "load detection results..." << std::endl;
	std::vector<std::vector<std::string>>  vv;
	kb::load_csv(path_obj, vv, ',');
	int num_vv = vv.size();
	int num_frame = num_vv - 1;

	std::vector<std::vector<cv::Point3i>> vm;
	vm.resize(num_frame);
	std::cout << "frame number: " << num_frame << std::endl;

	for (int k = 1; k <= num_frame; k++) {
		int num_v = vv[k].size();
		if (num_v <= 1)
			continue;
		int num_item = (num_v - 1) / 5;
		for (int kk = 0; kk < num_item; kk++) {
			std::string name = vv[k][kk * 5 + 1];
			int x1 = std::stoi(vv[k][kk * 5 + 2]);
			int y1 = std::stoi(vv[k][kk * 5 + 3]);
			int x2 = std::stoi(vv[k][kk * 5 + 4]);
			int y2 = std::stoi(vv[k][kk * 5 + 5]);
			int x = (x1 + x2) / 2;
			int y = (y1 + y2) / 2;


			std::map<std::string, int>::iterator it = lut_class.find(name);
			if (it != lut_class.end()) {

				vm[k - 1].push_back(cv::Point3i(x, y, it->second));
			}
		}
	}


	//	tracking
	std::cout << "tracking..." << std::endl;
	std::vector< std::vector<int>> vv_lut12;
	{
		vv_lut12.resize(num_frame);
		for (int k = 0; k < num_frame ; k++) {
			int num_item1 = vm[k].size();
			vv_lut12[k].resize(num_item1, -1);
		}

		int thres2 = threshold_d1 * threshold_d1;
		for (int k = 0; k < num_frame - 1; k++) {
			int num_item1 = vm[k].size();
			int num_item2 = vm[k + 1].size();

			std::cout << "--- " << k << " ---- " << num_item1 << " "<<num_item2 << std::endl;

			for(int j=0;j<num_class;j++){
				//std::cout << *it_item << std::endl;

				std::vector<int> lut1, lut2;
				lut1.resize(num_item1, -1);
				lut2.resize(num_item2, -1);


				//	forward search
				{
					for (int i1 = 0; i1 < num_item1; i1++) {
						if (j != vm[k][i1].z)
							continue;

						int min_d2 = -1;
						for (int i2 = 0; i2 < num_item2; i2++) {
							if (j != vm[k + 1][i2].z)
								continue;

							cv::Point3i d = vm[k][i1] - vm[k + 1][i2];
							int d2 = d.x * d.x + d.y * d.y;
							//	distance threshold
							if (thres2 < d2)
								continue;
							//	find an object to minimize distance
							if (min_d2 < 0 || d2 < min_d2) {
								lut1[i1] = i2;
							}
						}
					}
				}
				//	backward search
				{
					for (int i2 = 0; i2 < num_item2; i2++) {
						if (j != vm[k + 1][i2].z)
							continue;
						int min_d2 = -1;

						for (int i1 = 0; i1 < num_item1; i1++) {
							if (j != vm[k][i1].z)
								continue;

							cv::Point3i d = vm[k][i1] - vm[k + 1][i2];
							int d2 = d.x * d.x + d.y * d.y;
							//	distance threshold
							if (thres2 < d2)
								continue;
							//	find an object to minimize distance
							if (min_d2 < 0 || d2 < min_d2) {
								lut2[i2] = i1;
							}
						}
					}
				}

				//	check reciprocal consistency between forward and backward search
				{
					for (int i1 = 0; i1 < num_item1; i1++) {
						if (j != vm[k][i1].z)
							continue;

						int i2 = lut1[i1];
						if (i2 < 0)
							continue;
						int i1x = lut2[i2];
						if (i1x < 0)
							continue;
						if (i1x == i1) {
							vv_lut12[k][i1] = i2;
						}
					}
				}
			}
		}
	}
	if (debug > 0) {
		getchar();
	}

	//	pick up tracking sequence
	std::vector< std::vector<cv::Point>> vv_seq;
	{
		for (int k = 0; k < num_frame; k++) {
			int num = vv_lut12[k].size();
			for (int i1 = 0; i1 < num; i1++) {
				int i2 = vv_lut12[k][i1];
				if (i2 < 0)
					continue;
				int k1 = k;

				std::vector<cv::Point> v_seq;
				v_seq.push_back(cv::Point(k1, i1));
				while (1) {
					k1++;
					v_seq.push_back(cv::Point(k1, i2));

					if ((num_frame - 1) <= k1)
						break;

					int i3 = vv_lut12[k1][i2];
					if (i3 < 0)
						break;
					i2 = i3;
				}
				int num_seq = v_seq.size();
				for (int i = 0; i < num_seq; i++) {
					//std::cout << v_seq[i] << " ";

					vv_lut12[v_seq[i].x][v_seq[i].y] = -1;
				}
				vv_seq.push_back(v_seq);
			}
		}
	}
	if (debug > 0) {
		getchar();
	}

	//	save tracking sequence
	std::cout << "save tracking sequence..." << std::endl;
	{
		if (threshold_num_seq < 1) {
			threshold_num_seq = 1;
		}
		int num_vv_seq = vv_seq.size();
		for (int k = 0; k < num_vv_seq; k++) {
			int num_seq = vv_seq[k].size();
			std::cout << k << " / " << num_vv_seq <<": "<<num_seq<< std::endl;
			if (num_seq < threshold_num_seq)
				continue;
			int f0 = vv_seq[k][0].x;
			int i0 = vv_seq[k][0].y;
			int num_obj0=vm[f0].size();
			if (num_obj0 <= i0) {
				std::cout << "unexpected error: " << f0 << " " << i0 << std::endl;
				continue;
			}

			char buf[128];
			snprintf(buf, 128, "%05d_", k);
			std::cout << v_class[vm[f0][i0].z] << std::endl;
			std::string path1 = dname_out + buf + v_class[vm[f0][i0].z]+".txt";
			std::cout << path1 << std::endl;
			std::ofstream file(path1);

			for (int i = 0; i < num_seq ; i++) {
				int f1 = vv_seq[k][i].x;
				int i1 = vv_seq[k][i].y;

				file << f1 << " " << vm[f1][i1].x << " " << vm[f1][i1].y << std::endl;
			}
		}

	}


	std::cout << "save overlay images..." << std::endl;
	if (mode_save_images > 0) {

		//	input image list
		std::vector< std::string > filenames;
		if (kb::search_files(dname_images, search_key.c_str(), filenames) < 0)
			return -1;

		int	num_files = filenames.size();
		std::cout << "number of files = " << num_files << std::endl;
		std::vector<cv::Mat> vmat;
		vmat.resize(num_files);
		for (int k = 0; k < num_files; k++) {
			std::string path1 = dname_images + filenames[k];
			vmat[k] = cv::imread(path1);
		}

		//int num_vv_seq = vv_seq.size();
		//for (int k = 0; k < num_vv_seq; k++) {
		//	int num_seq = vv_seq[k].size();
		//	if (num_seq == 0)
		//		continue;
		//	std::cout << vv_seq[k][0].x << ": ";
		//	for (int i = 0; i < num_seq; i++) {
		//		std::cout << vv_seq[k][i].y << " ";
		//		vmat[k]
		//	}
		//}
		int dvd = 2;
		int num_vv_seq = vv_seq.size();
		for (int k = 0; k < num_vv_seq; k++) {
			int num_seq = vv_seq[k].size();
			std::cout << k << " / " << num_vv_seq << std::endl;
			if (num_seq == 0)
				continue;
			for (int i = 0; i < num_seq - 1; i++) {
				int f1 = vv_seq[k][i].x;
				int i1 = vv_seq[k][i].y;
				int f2 = vv_seq[k][i + 1].x;
				int i2 = vv_seq[k][i + 1].y;

				cv::Point p1(vm[f1][i1].x / dvd, vm[f1][i1].y / dvd);
				cv::Point p2(vm[f2][i2].x / dvd, vm[f2][i2].y / dvd);

				for (int ii = 0; ii < num_seq; ii++) {
					int f = vv_seq[k][ii].x;
					cv::line(vmat[f], p1, p2, cv::Scalar(0, 255, 0), 3);
				}
			}
		}
		for (int k = 0; k < num_vv_seq; k++) {
			int num_seq = vv_seq[k].size();
			if (num_seq == 0)
				continue;
			for (int i = 0; i < num_seq; i++) {
				int f1 = vv_seq[k][i].x;
				int i1 = vv_seq[k][i].y;
				cv::Point p1(vm[f1][i1].x / dvd, vm[f1][i1].y / dvd);
				cv::circle(vmat[f1], p1, 7, cv::Scalar(0, 0, 255), 5);
			}
		}

		for (int k = 0; k < num_files; k++) {
			std::string path1 = dname_out + filenames[k];
			cv::imwrite(path1, vmat[k]);
		}
	}


    return 0;
}

