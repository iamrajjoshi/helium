// Copyright (C) 2026 The personal Helium fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chrome/browser/ui/tabs/github_live_folder_model.h"

#include <climits>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::size_t assertions = 0;
std::size_t failures = 0;

void Check(bool condition, const char* expression, int line) {
  ++assertions;
  if (!condition) {
    ++failures;
    if (failures <= 30) {
      std::cerr << "FAIL line " << line << ": " << expression << '\n';
    }
  }
}

#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

bool IsAsciiAlphanumeric(unsigned char value) {
  return (value >= 'a' && value <= 'z') ||
         (value >= 'A' && value <= 'Z') ||
         (value >= '0' && value <= '9');
}

void TestAcceptedRepositoryUrls() {
  using helium::ParseGitHubRepositoryUrl;
  const std::vector<std::string> suffixes = {
      "", "/", "/pulls", "/pull/123", "/tree/main/path/to/file.cc",
      "?tab=readme-ov-file", "#readme", "?#", "/?tab=readme#top",
      "/tree/feature%2Fbranch", "?redirect=https://evil.example/a/b",
      "#https://not-github.example", "/a%20b?encoded=%5c%00#fragment"};
  for (const auto& suffix : suffixes) {
    const auto parsed =
        ParseGitHubRepositoryUrl("https://github.com/Octo-Cat/My_Repo.git" +
                                 suffix);
    CHECK(parsed.has_value());
    if (parsed) {
      CHECK(parsed->owner == "Octo-Cat");
      CHECK(parsed->name == "My_Repo.git");
      CHECK(parsed->Key() == "octo-cat/my_repo.git");
    }
  }

  for (const std::string name : {"a", "1", "-", "_", "...", ".github",
                                  "repo.name-1_2", "repo.git"}) {
    const auto parsed = ParseGitHubRepositoryUrl("https://github.com/a/" + name);
    CHECK(parsed.has_value());
    if (parsed) {
      CHECK(parsed->name == name);
    }
  }
  CHECK(ParseGitHubRepositoryUrl("https://github.com/0/0"));
  CHECK(ParseGitHubRepositoryUrl("https://github.com/a-b-c/repo"));
  const auto largest = ParseGitHubRepositoryUrl(
      "https://github.com/" + std::string(39, 'A') + "/" +
      std::string(100, 'R'));
  CHECK(largest.has_value());
  if (largest) {
    CHECK(largest->owner.size() == 39);
    CHECK(largest->name.size() == 100);
  }
}

void TestRejectedRepositoryUrls() {
  using helium::ParseGitHubRepositoryUrl;
  const std::vector<std::string> invalid = {
      "", "github.com/a/b", "//github.com/a/b", "http://github.com/a/b",
      "HTTPS://github.com/a/b", "https://GitHub.com/a/b",
      "https://GITHUB.COM/a/b", "https:/github.com/a/b",
      "https:///github.com/a/b", "https://github.com", "https://github.com/",
      "https://github.com/a", "https://github.com/a/", "https://github.com/a//",
      "https://github.com//b", "https://github.com/a?b/c",
      "https://github.com/a#b/c", "https://github.com/a/?query=b",
      "https://github.com/a/#b", "https://github.com./a/b",
      "https://github.com:443/a/b", "https://github.com:80/a/b",
      "https://github.com:/a/b", "https://www.github.com/a/b",
      "https://api.github.com/a/b", "https://github.com.evil.example/a/b",
      "https://github.comevil.example/a/b", "https://evilgithub.com/a/b",
      "https://github.com@evil.example/a/b", "https://user@github.com/a/b",
      "https://user:password@github.com/a/b", "https://@github.com/a/b",
      "https://github%2ecom/a/b", "https://%67ithub.com/a/b",
      "https://github.com%2fevil.example/a/b", "https://github.com%00/a/b",
      "https://github.com\\@evil.example/a/b", "https://github.com/a\\b/c",
      "https://github.com/a/b\\c", "https://github.com/a/b?query=\\evil",
      "https://github.com/a/b#\\evil", "https://github.com/a/b/\\evil",
      "https://github.com/./b", "https://github.com/../b",
      "https://github.com/-owner/b", "https://github.com/owner-/b",
      "https://github.com/a--b/repo", "https://github.com/a_b/repo",
      "https://github.com/a.b/repo", "https://github.com/-/repo",
      "https://github.com/a/.", "https://github.com/a/..",
      "https://github.com/a/repo:123", "https://github.com/a/repo@evil",
      "https://github.com/a/%2e", "https://github.com/a/%2e%2e",
      "https://github.com/%41/b", "https://github.com/a/%42",
      "https://github.com/a%2fb/c", "https://github.com/a/b%2fc",
      "https://github.com/a%3fb/c", "https://github.com/a/b%3fc",
      "https://github.com/a%23b/c", "https://github.com/a/b%23c",
      "https://github.com/a/%00", "https://github.com/a/%5c",
      "https://github.com/a/b%", "https://github.com/a/b%ZZ",
      "https://github.com/a/b%252f", " https://github.com/a/b",
      "https://github.com/a/b ", "https://github.com/a/b\n",
      "javascript:https://github.com/a/b", "file://github.com/a/b",
      "https://github\xef\xbc\x8e" "com/a/b",
      "https://g\xd1\x96" "thub.com/a/b",
      "https://github.com/\xc3\xa9/b", "https://github.com/a/\xc3\xa9",
      "https://github.com/a\xef\xbc\x8f" "b/c"};
  for (const auto& url : invalid) {
    CHECK(!ParseGitHubRepositoryUrl(url));
  }
  CHECK(!ParseGitHubRepositoryUrl("https://github.com/" + std::string(40, 'a') +
                                  "/repo"));
  CHECK(!ParseGitHubRepositoryUrl("https://github.com/a/" +
                                  std::string(101, 'b')));
}

void TestAllByteAllowlists() {
  using helium::GitHubRepository;
  using helium::ParseGitHubRepositoryUrl;
  for (unsigned int byte = 0; byte <= 255; ++byte) {
    const char value = static_cast<char>(byte);
    const std::string segment = std::string("a") + value + "b";
    const bool valid_owner = IsAsciiAlphanumeric(byte) || value == '-';
    const bool valid_name = valid_owner || value == '.' || value == '_';
    CHECK(!GitHubRepository({segment, "repo"}).ApiUrl().empty() == valid_owner);
    CHECK(!GitHubRepository({"owner", segment}).ApiUrl().empty() == valid_name);

    // These three delimiters have path/query meaning, not segment meaning.
    // Their parsing behavior is covered explicitly in the URL corpus.
    if (value != '/' && value != '?' && value != '#') {
      const auto owner = ParseGitHubRepositoryUrl(
          "https://github.com/" + segment + "/repo");
      const auto name = ParseGitHubRepositoryUrl(
          "https://github.com/owner/" + segment);
      CHECK(owner.has_value() == valid_owner);
      CHECK(name.has_value() == valid_name);
    }

    if (byte <= 0x20 || byte == 0x7f || value == '\\') {
      // Check the entire URL, including otherwise ignored query/fragment data.
      for (const std::string prefix : {"/child", "?query=value", "#anchor"}) {
        CHECK(!ParseGitHubRepositoryUrl("https://github.com/a/b" + prefix +
                                       value + "suffix"));
      }
    }
  }
}

void TestRepositoryIdentityAndGeneratedUrls() {
  const helium::GitHubRepository repo{"Octo-Cat", "Hello_World.git"};
  const helium::GitHubRepository same{"octo-cat", "hello_world.GIT"};
  CHECK(repo == same);
  CHECK(!(repo != same));
  CHECK(repo.Key() == "octo-cat/hello_world.git");
  CHECK(repo.Key() == same.Key());
  CHECK(repo != helium::GitHubRepository({"other", repo.name}));
  CHECK(repo != helium::GitHubRepository({repo.owner, "other"}));
  CHECK(repo.ApiUrl() ==
        "https://api.github.com/repos/Octo-Cat/Hello_World.git/"
        "pulls?state=open&sort=updated&direction=desc&per_page=10");
  static_assert(helium::kGitHubLiveFolderMaxPullRequests == 10);
  CHECK(repo.PullUrl(1) ==
        "https://github.com/Octo-Cat/Hello_World.git/pull/1");
  CHECK(repo.PullUrl(INT_MAX) ==
        "https://github.com/Octo-Cat/Hello_World.git/pull/" +
            std::to_string(INT_MAX));
  CHECK(repo.PullUrl(0).empty());
  CHECK(repo.PullUrl(-1).empty());
  CHECK(repo.PullUrl(INT_MIN).empty());
  for (const helium::GitHubRepository& invalid :
       std::vector<helium::GitHubRepository>{{"", "repo"}, {"owner", ""},
                                            {"a/b", "repo"}, {"a", "../repo"},
                                            {"a@evil", "b"}, {"a", "%2f"}}) {
    CHECK(invalid.ApiUrl().empty());
    CHECK(invalid.PullUrl(1).empty());
    CHECK(!helium::IsExpectedGitHubPullUrl(invalid, 1,
                                         "https://github.com/a/b/pull/1"));
  }
}

void TestCanonicalPullRequestUrls() {
  using helium::IsExpectedGitHubPullUrl;
  const helium::GitHubRepository repo{"Owner", "Repo"};
  for (const int number : {1, 2, 42, 999999, INT_MAX}) {
    CHECK(IsExpectedGitHubPullUrl(repo, number, repo.PullUrl(number)));
    CHECK(IsExpectedGitHubPullUrl(
        repo, number, "https://github.com/oWNER/rEPO/pull/" +
                          std::to_string(number)));
    CHECK(!IsExpectedGitHubPullUrl(repo, 0, repo.PullUrl(number)));
    CHECK(!IsExpectedGitHubPullUrl(repo, -number, repo.PullUrl(number)));
    CHECK(!IsExpectedGitHubPullUrl(repo, number, repo.PullUrl(number) + "/"));
    CHECK(!IsExpectedGitHubPullUrl(repo, number, repo.PullUrl(number) + "?"));
    CHECK(!IsExpectedGitHubPullUrl(repo, number, repo.PullUrl(number) + "#"));
  }
  const std::vector<std::string> invalid = {
      "https://github.com/Owner/Repo/pull/2",
      "https://github.com/Owner/Repo/pull/01",
      "https://github.com/Owner/Repo/pull/+1",
      "https://github.com/Owner/Repo/pull/-1",
      "https://github.com/Owner/Repo/pull/1.0",
      "https://github.com/Owner/Repo/pull/1e0",
      "https://github.com/Owner/Repo/pull/0x1",
      "https://github.com/Owner/Repo/pull/%31",
      "https://github.com/Owner/Repo/pull/1%00",
      "https://github.com/Owner/Repo/pull/1?tab=files",
      "https://github.com/Owner/Repo/pull/1#issuecomment-123",
      "https://github.com/Owner/Repo/pull/1/files",
      "https://github.com/Owner/Repo/PULL/1",
      "https://github.com/Owner/Repo/pulls/1",
      "https://github.com/Owner/Repo/issues/1",
      "https://github.com/Owner/Repo//pull/1",
      "https://github.com/Owner/Repo/pull//1",
      "https://github.com/Owner/Repo/./pull/1",
      "https://github.com/Owner/Repo/pull/../pull/1",
      "https://github.com/Other/Repo/pull/1",
      "https://github.com/Owner/Other/pull/1",
      "https://github.com/%4fwner/Repo/pull/1",
      "https://github.com/Owner/%52epo/pull/1",
      "https://github.com/Owner/Repo/pull/2147483648",
      "https://github.com/Owner/Repo/pull/18446744073709551616",
      "http://github.com/Owner/Repo/pull/1",
      "https://GitHub.com/Owner/Repo/pull/1",
      "https://github.com:443/Owner/Repo/pull/1",
      "https://github.com.evil.example/Owner/Repo/pull/1",
      "https://github.com@evil.example/Owner/Repo/pull/1",
      "https://evil@github.com/Owner/Repo/pull/1",
      "https://github.com/Owner/Repo/pull/1\n"};
  for (const auto& url : invalid) {
    CHECK(!IsExpectedGitHubPullUrl(repo, 1, url));
  }
  CHECK(!IsExpectedGitHubPullUrl(repo, INT_MIN, ""));
  CHECK(!IsExpectedGitHubPullUrl(repo, 0, ""));
  CHECK(!IsExpectedGitHubPullUrl(repo, 1, ""));
  const std::string nul_url = repo.PullUrl(1) + std::string(1, '\0') + "evil";
  CHECK(!IsExpectedGitHubPullUrl(repo, 1, nul_url));
}

void TestRefreshGenerationOrdering() {
  helium::GitHubLiveFolderRefreshGuard guard;
  CHECK(!guard.IsCurrent(0));
  CHECK(!guard.IsCurrent(1));
  std::uint64_t previous = 0;
  for (std::uint64_t i = 1; i <= 10000; ++i) {
    const auto current = guard.Begin();
    CHECK(current == i);
    CHECK(guard.IsCurrent(current));
    CHECK(!guard.IsCurrent(previous));
    CHECK(!guard.IsCurrent(current + 1));
    CHECK(!guard.IsCurrent(0));
    previous = current;
  }
  // Model an old HTTP response arriving after a newer refresh was requested.
  const auto old_request = guard.Begin();
  const auto new_request = guard.Begin();
  CHECK(!guard.IsCurrent(old_request));
  CHECK(guard.IsCurrent(new_request));
  CHECK(!guard.IsCurrent(UINT64_MAX));
}

void TestStoppedRefreshGuard() {
  helium::GitHubLiveFolderRefreshGuard guard;
  const auto first = guard.Begin();
  const auto latest = guard.Begin();
  guard.Stop();
  CHECK(!guard.IsCurrent(first));
  CHECK(!guard.IsCurrent(latest));
  CHECK(!guard.IsCurrent(0));
  CHECK(!guard.IsCurrent(UINT64_MAX));
  for (int i = 0; i < 100; ++i) {
    const auto disabled = guard.Begin();
    CHECK(disabled == 0);
    CHECK(!guard.IsCurrent(disabled));
    CHECK(!guard.IsCurrent(latest));
    guard.Stop();
  }
  helium::GitHubLiveFolderRefreshGuard never_started;
  never_started.Stop();
  CHECK(never_started.Begin() == 0);
  CHECK(!never_started.IsCurrent(0));
  CHECK(!never_started.IsCurrent(1));

  // Closing one live folder must not disable an unrelated folder's refreshes.
  helium::GitHubLiveFolderRefreshGuard independent;
  CHECK(independent.Begin() == 1);
  CHECK(independent.IsCurrent(1));
  CHECK(!guard.IsCurrent(1));
}

}  // namespace

int main() {
  TestAcceptedRepositoryUrls();
  TestRejectedRepositoryUrls();
  TestAllByteAllowlists();
  TestRepositoryIdentityAndGeneratedUrls();
  TestCanonicalPullRequestUrls();
  TestRefreshGenerationOrdering();
  TestStoppedRefreshGuard();
  if (failures) {
    std::cerr << failures << " failed of " << assertions << " assertions\n";
    return 1;
  }
  std::cout << "PASS: 7 groups, " << assertions
            << " assertions (GitHub Live Folder native model)\n";
  return 0;
}
