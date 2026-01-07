#include "camera_instance.h"
#include "dev_vi_os04a10_liner.h"
#include "dev_vi_os08a20_2to1wdr.h"

void camera_instance::init_vi()
{
    // Check and release any existing VI component
    if (m_vi_ptr)
    {
        delete m_vi_ptr;
        m_vi_ptr = nullptr;
    }

    // Select the VI component based on camera configuration
    const auto& sensor_type = m_camera_config.sensor_type;
    const auto& work_mode = m_camera_config.work_mode;

    if (sensor_type == "os04a10" && work_mode == "linear")
    {
        m_vi_ptr = new dev_vi_os04a10_liner(m_camera_config);
    }
    else if (sensor_type == "os08a20" && work_mode == "2to1wdr")
    {
        m_vi_ptr = new dev_vi_os08a20_2to1wdr(m_camera_config);
    }
    else
    {
        throw std::runtime_error("Unsupported sensor type or work mode.");
    }

    // Ensure the VI component is successfully initialized
    if (!m_vi_ptr || !m_vi_ptr->initialize())
    {
        throw std::runtime_error("Failed to initialize the VI component.");
    }
}