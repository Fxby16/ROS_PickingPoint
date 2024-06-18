#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <image_transport/image_transport.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <std_msgs/msg/int32_multi_array.hpp>

#include <pickingPoint.hpp>
#include <timer.hpp>

class PickingPointHandler : public rclcpp::Node
{
public:
    static std::shared_ptr<PickingPointHandler> Create()
    {
        auto node = std::make_shared<PickingPointHandler>();
        node->Init();
        return node;
    }

    PickingPointHandler() : Node("picking_point_handler") {}

    void Init()
    {
        // Initialize the image transport subscriber
        image_transport::ImageTransport it(shared_from_this());
		m_Pub = this->create_publisher<std_msgs::msg::Int32MultiArray>("pickingPoint/coordinates", 10);
        m_Sub = it.subscribe("camera/image", 10, std::bind(&PickingPointHandler::ImageCallback, this, std::placeholders::_1));
    }

    void ImageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& msg)
    {
        try
        {
            // Convert ROS2 message to OpenCV image
            cv::Mat image = cv_bridge::toCvShare(msg, "bgr8")->image;

            // Start timer for performance measurement
            Timer* timer = new Timer;

            // Find the picking point
            PickingPoint pp(image);
            cv::Point pickingPoint = pp.Process();

            delete timer;

			std_msgs::msg::Int32MultiArray array;
			// Set up dimensions
			array.layout.dim.push_back(std_msgs::msg::MultiArrayDimension());
			array.layout.dim[0].size = 2;
			array.layout.dim[0].stride = 2;
			array.layout.dim[0].label = "x_y_coordinates";
			// Assign the data
			array.data.clear();
			array.data.push_back(pickingPoint.x);
			array.data.push_back(pickingPoint.y);

            #ifdef DEBUG
                RCLCPP_INFO(this->get_logger(), "Picking point: (%d, %d)", pickingPoint.x, pickingPoint.y);
            #endif

            m_Pub->publish(array);
        }
        catch(cv_bridge::Exception& e)
        {	
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr m_Pub;
    image_transport::Subscriber m_Sub;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    cv::namedWindow("view");
    rclcpp::spin(PickingPointHandler::Create());
    cv::destroyWindow("view");
    rclcpp::shutdown();
    return 0;
}