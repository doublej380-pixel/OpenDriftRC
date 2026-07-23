document.documentElement.classList.add("js");

const installButtons = document.querySelectorAll("esp-web-install-button");

fetch("./release.json")
  .then((response) => {
    if (!response.ok) {
      throw new Error(`Release metadata returned ${response.status}`);
    }

    return response.json();
  })
  .then((release) => {
    document.querySelectorAll("[data-release-version]").forEach((element) => {
      element.textContent = release.version;
    });
  })
  .catch(() => {
    // The checked-in fallback version remains visible when metadata is
    // unavailable, including when index.html is opened directly from disk.
  });

installButtons.forEach((installer) => {
  installer.addEventListener("state-changed", (event) => {
    const state = event.detail?.state;

    if (state) {
      installer.dataset.state = state;
    }
  });
});
