#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/program_options.hpp>

#include <opencv2/opencv.hpp>

namespace {

class BgCut {
 public:
  explicit BgCut(std::string_view path) : image_(cv::imread(path.data())) {
    std::stringstream ss;
    ss << path << ".bgcut.png";
    output_image_path_ = ss.str();
  }

  void Run() {
    cv::namedWindow(kWindowTitle, cv::WINDOW_NORMAL);
    cv::resizeWindow(kWindowTitle, 500, 500);
    cv::setMouseCallback(kWindowTitle, MouseCallback, static_cast<void*>(this));
    cv::imshow(kWindowTitle, image_);
    while (!ProcessKeyPress(cv::waitKey())) {}
  }

 private:
  constexpr static char const* kWindowTitle = "BgCut powered by GrabCut OpenCV";
  static inline const cv::Scalar kBackground = {cv::GC_BGD};
  static inline const cv::Scalar kForeground = {cv::GC_FGD};
  static inline const cv::Scalar kSelectedRegionColor = {110, 250, 110};

  static void MouseCallback(int event, int x, int y, int flags, void *self) {
    BgCut *this_obj = static_cast<BgCut*>(self);
    this_obj->ProcessMouseEvent(event, x, y, flags);
  }

  template <typename F,
            typename = 
                std::enable_if_t<std::is_invocable_v<F, cv::Mat const&>>>
  void MaskImageAndThen(F&& action) {
    cv::Mat bin_mask = mask_ & 1;
    cv::Mat masked_image;
    image_.copyTo(masked_image, bin_mask);
    action(masked_image);
  }

  void DrawMaskedImage() {
    MaskImageAndThen(
        [](cv::Mat const& masked_image) {
          cv::imshow(kWindowTitle, masked_image);
        });
  }

  void SaveMaskedImageWithTransparentBg() {
    MaskImageAndThen(
        [this](cv::Mat const& masked_image) {
          cv::Mat gray_image;
          cv::cvtColor(masked_image, gray_image, cv::COLOR_BGR2GRAY);
          cv::Mat alpha_channel;
          cv::threshold(gray_image, alpha_channel, 0, 255, cv::THRESH_BINARY);
          std::vector<cv::Mat> channels;
          cv::split(masked_image, channels);
          channels.push_back(std::move(alpha_channel));
          cv::Mat image_with_transparency;
          cv::merge(channels, image_with_transparency);
          cv::imwrite(output_image_path_, image_with_transparency);
        });
  }

  bool ProcessKeyPress(int key_pressed) {
    bool should_exit = false;
    switch (key_pressed) {
    case 'c':
      mask_.setTo(kForeground);
      DrawMaskedImage();
      mask_ = cv::Mat();
      break;
    case 's':
      SaveMaskedImageWithTransparentBg();
      break;
    case 'n':
      if (!mask_.empty()) {
        cv::grabCut(image_, mask_, {}, bg_model_, fg_model_, 1);
        DrawMaskedImage();
      }
      break;
    case 'q':
      should_exit = true;
      break;
    }
    return should_exit;
  }

  void ProcessMouseEvent(int event, int x, int y, int flags) {
    switch (event) {
    case cv::EVENT_MOUSEMOVE:
      if (start_point_.has_value() && !mask_.empty()) {
        if ((flags & cv::EVENT_FLAG_CTRLKEY) == cv::EVENT_FLAG_CTRLKEY)
          cv::circle(mask_, {x, y}, 1, kForeground, -1);
        else if ((flags & cv::EVENT_FLAG_SHIFTKEY) == cv::EVENT_FLAG_SHIFTKEY)
          cv::circle(mask_, {x, y}, 1, kBackground, -1);
      }
      break;
    case cv::EVENT_LBUTTONDOWN:
      start_point_ = {x, y};
      break;
    case cv::EVENT_LBUTTONUP:
      if (!mask_.empty()) {
        if ((flags & cv::EVENT_FLAG_CTRLKEY) == cv::EVENT_FLAG_CTRLKEY)
          cv::circle(mask_, {x, y}, 1, kForeground, -1);
        else if ((flags & cv::EVENT_FLAG_SHIFTKEY) == cv::EVENT_FLAG_SHIFTKEY)
          cv::circle(mask_, {x, y}, 1, kBackground, -1);
      } else {
        cv::Rect rect = {start_point_.value(), cv::Point{x, y}};
        if (rect.area() > 0) {
          cv::Mat image_to_draw_over = image_.clone();
          cv::rectangle(image_to_draw_over, rect, kSelectedRegionColor, 3);
          cv::imshow(kWindowTitle, image_to_draw_over);
          cv::grabCut(image_, mask_, rect, bg_model_, fg_model_, 1,
              cv::GC_INIT_WITH_RECT);
        }
      }
      start_point_.reset();
    }
  }

  cv::Mat image_;
  cv::Mat mask_;
  cv::Mat bg_model_, fg_model_;
  std::optional<cv::Point> start_point_;
  std::string output_image_path_;
};

} // namespace

int main(int argc, char** argv) {
  try {
    boost::program_options::options_description desc(
        "Interactively remove image background using the GrabCut algorithm");
    desc.add_options()
        ("image", boost::program_options::value<std::string>()->required(),
            "image file to remove background from");
    boost::program_options::variables_map options;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), options);
    boost::program_options::notify(options);
    auto bgcut = BgCut(options["image"].as<std::string>());
    bgcut.Run();
  } catch (std::exception& err) {
    std::cerr << err.what() << std::endl;
    return 1;
  }
  return 0;
}
