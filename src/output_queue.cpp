#include <vector>
#include <mutex>
#include <string>
#include <optional>
#include <cstdint>
#include <ctime>
#include <cstdio>
#include <thread>
#include <chrono>
#include <iostream>

#include "output_queue.h"
#include "wavfile.h"

enum class JobStatus {
    NEW,
    WAV_OUTPUT_DONE,

    FINISHED
};

struct OutputJob {
    JobStatus status;

    std::string filename;
    std::vector<int16_t> samples;
    int sample_rate;

    OutputJob(std::string filename, std::vector<int16_t> samples, int sample_rate) : 
        status(JobStatus::NEW),
        filename(filename),
        samples(samples),
        sample_rate(sample_rate)
    {}
};

std::mutex output_queue_mutex;
std::vector<OutputJob> output_queue;
bool should_quit_oq_thread = false;

std::string gen_filename() {
    // filename is based on current unix time
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char buffer[80];
    strftime(buffer, 80, "%Y%m%d-%H%M%S", timeinfo);
    std::string filename(buffer);
    filename = "captures/" + filename + "-" + std::to_string(rand() % 10000) + ".wav";

    return filename;
}

void output_queue_push(int16_t * samples, size_t sample_count, int sample_rate) {
    std::lock_guard<std::mutex> lock(output_queue_mutex);

    std::vector<int16_t> sample_vector(samples, samples + sample_count);
    std::string filename = gen_filename();

    output_queue.emplace_back(OutputJob(filename, sample_vector, sample_rate));
}


// module private
void oq_poll_thread() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::lock_guard<std::mutex> lock(output_queue_mutex);

        if (should_quit_oq_thread) {
            return;
        }

        for (auto & job : output_queue) {
            if (job.status == JobStatus::NEW) {
                job.status = JobStatus::WAV_OUTPUT_DONE;
                std::cout << "Writing " << job.filename << std::endl;
                write_wav(job.samples.data(), job.samples.size(), job.sample_rate, job.filename);
            } else if (job.status == JobStatus::WAV_OUTPUT_DONE) {
                job.status = JobStatus::FINISHED;
            }
        }

        output_queue.erase(
            std::remove_if(
                output_queue.begin(), 
                output_queue.end(), 
                [](OutputJob & job) { return job.status == JobStatus::FINISHED; }
            ),
            output_queue.end()
        );
    }
}

void output_queue_start_thread() {
    std::thread t(oq_poll_thread);
    t.detach();
}