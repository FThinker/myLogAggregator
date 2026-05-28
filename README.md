<a id="readme-top"></a>
<div align="center">
  
  [![Contributors][contributors-shield]][contributors-url]
  [![Forks][forks-shield]][forks-url]
  [![Stargazers][stars-shield]][stars-url]
  [![Issues][issues-shield]][issues-url]
  [![MIT License][license-shield]][license-url]
  [![C][c-shield]][c-url]

  <br />
  <h1 align="center"><b>myLogAggregator</b></h1>

  <p align="center">
    A custom C distributed log aggregator developed as a group project for the exam "Sistemi Operativi II" in Sapienza's Bachelor of Computer Science course.
    <br />
    <br />
    <a href="https://github.com/fthinker/mylogaggregator/issues">Report a bug</a>
    ·
    <a href="https://github.com/fthinker/mylogaggregator/issues">Request new features</a>
  </p>
</div>



## About The Project

**myLogAggregator** is a custom-built distributed log aggregator designed from the ground up as an academic homework. This project is currently being developed for the *Sistemi Operativi II* (Operating Systems II) exam at Sapienza University of Rome, Bachelor of Computer Science.

The system handles distributed log collection, implementing core functionalities such as concurrency handling, log rotation, and graceful shutdown, ensuring robust collection and organization of logs from multiple sources.

## Getting Started

To get a local copy up and running, follow these steps.

### Prerequisites

This project strictly requires the **GNU Compiler Collection (GCC)** to compile and run. A Unix-like environment is also required to run the provided shell test scripts. Ensure you have it installed on your system before proceeding.

### Installation

1. Clone the repo
```sh
git clone https://github.com/fthinker/myLogAggregator/
```


2. Navigate to the project directory
```sh
cd myLogAggregator
```


3. Compile the server with the `-pthread` option
```sh
gcc src/coordinator.c src/logger.c src/network.c -pthread -o coordinator
```

4. Compile the client
```sh
gcc src/network.c src/producer.c -o producer
```


## Usage

You can either launch both the main aggregator daemon and the singular client producers manually or simply run the existing test files.

**Run the test scripts:**
To run the already present tests, simply do the following:

* **Test 1 - Concurrency:**
```sh
./test1_concurrency.sh
```

* **Test 2 - Log Rotation:**
```sh
./test2_rotation.sh
```

* **Test 3 - Graceful Shutdown:**
```sh
./test3_shutdown.sh
```

## Additional Flags

**Coordinator: **
The coordinator server can be run with the additional parameter "-d" or "--debug" to allow verbose printing of its operations.
```sh
./coordinator -d
# or
./coordinator --debug
```

**Producer: **
The producer client accepts two additional parameters: "-d" or "--debug" for verbose, "-r <value>" or "--random <value>" to enable the generation of a random number of requests (at least 1, at most <value>: <value> is optional, default is 5).

```sh
./producer -d   # start the producer in debug mode.
```

```sh
./producer -d -r   # start the producer in debug mode and send a random (1-5) number of requests.
```

```sh
./producer -r 10   # start the producer and send a random (1-10) number of requests.
```


## Known Issues

This program was built for a specific academic assignment. As such, it might not handle extreme edge cases like massive industrial-scale throughput or severe network partitions.
It is also prone to surpass the maximum log filesize limit by about 20%-30% before successfully rotating log files.
Overall, long throughput-intensive bursts with thousands of producers are supported, albeit not recommended.

## Contributors

As a group project, contributors are limited to the existing team members. However, feedback and suggestions are always welcome and appreciated!

## Star History

<a href="https://www.star-history.com/?repos=FThinker%2FmyLogAggregator&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=FThinker/myLogAggregator&type=date&theme=dark&legend=bottom-right" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=FThinker/myLogAggregator&type=date&legend=bottom-right" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=FThinker/myLogAggregator&type=date&legend=bottom-right" />
 </picture>
</a>


## License

Distributed under the MIT License. See `LICENSE` for more information.

[contributors-shield]: https://img.shields.io/github/contributors/fthinker/mylogaggregator.svg?style=for-the-badge
[contributors-url]: https://github.com/fthinker/mylogaggregator/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/fthinker/mylogaggregator.svg?style=for-the-badge
[forks-url]: https://github.com/fthinker/mylogaggregator/network/members
[stars-shield]: https://img.shields.io/github/stars/fthinker/mylogaggregator.svg?style=for-the-badge
[stars-url]: https://github.com/fthinker/mylogaggregator/stargazers
[issues-shield]: https://img.shields.io/github/issues/fthinker/mylogaggregator.svg?style=for-the-badge
[issues-url]: https://github.com/fthinker/mylogaggregator/issues
[license-shield]: https://img.shields.io/github/license/fthinker/mylogaggregator.svg?style=for-the-badge
[license-url]: https://github.com/fthinker/mylogaggregator/blob/main/LICENSE
[c-shield]: https://img.shields.io/badge/LANGUAGE-C-00599C?style=for-the-badge
[c-url]: https://en.wikipedia.org/wiki/C_(programming_language)
