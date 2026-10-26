#pragma once
#include <ResponseParser.h>
#include <ctime>
#include <fstream>
#include <functional>

namespace weasel {

// IPC 反序列化失败时写日志，不再弹模态框：原来的 MessageBoxA 会阻塞输入
// 线程，表现为输入法卡死并反复弹窗。
inline void _LogIpcError(const std::string& msg) {
  char dir[260] = {0};
  DWORD len = ::GetTempPathA(260, dir);
  if (len == 0 || len >= 260)
    return;
  std::string path(dir);
  path += "weasel-ipc.log";
  std::ofstream ofs(path.c_str(), std::ios::app);
  if (!ofs)
    return;
  time_t now = ::time(nullptr);
  struct tm tmv = {0};
  localtime_s(&tmv, &now);
  char stamp[32] = {0};
  strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
  ofs << stamp << " " << msg << std::endl;
}

template <typename T>
void TryDeserialize(boost::archive::text_wiarchive& ia, T& t) {
  try {
    ia >> t;
  } catch (const boost::archive::archive_exception& e) {
    _LogIpcError(std::string("boost::archive::archive_exception: ") + e.what());
  }
}
class Deserializer {
 public:
  typedef std::vector<std::wstring> KeyType;
  typedef std::shared_ptr<Deserializer> Ptr;
  typedef std::function<Ptr(ResponseParser* pTarget)> Factory;

  Deserializer(ResponseParser* pTarget) : m_pTarget(pTarget) {}
  virtual ~Deserializer() {}
  virtual void Store(KeyType const& key, std::wstring const& value) {}

  static void Initialize(ResponseParser* pTarget);
  static void Define(std::wstring const& action, Factory factory);
  static bool Require(std::wstring const& action, ResponseParser* pTarget);

 protected:
  ResponseParser* m_pTarget;

 private:
  static std::map<std::wstring, Factory> s_factories;
};

}  // namespace weasel
