# Đồ Án Giữa Kỳ: Cài Đặt Lại Lệnh `ls(1)` trên NetBSD 10.1

## 1. Thông tin sinh viên & Đồ án
- **Họ và tên:** Nguyễn Thành Đô
- **Mã sinh viên:** 24IT335
- **Môn học:** Lập trình Hệ thống / Hệ điều hành
- **Repository URL:** `https://github.com/Renber-vku/NguyenThanhDo_24IT24IT335_midterm.git`
- **Môi trường xác nhận:** NetBSD 10.1 (chuẩn POSIX.2 / NetBSD `ls(1)` Manual)

---

## 2. Bảng các tùy chọn hỗ trợ (Supported Options)

Chương trình cài đặt đầy đủ và chính xác toàn bộ các tùy chọn theo NetBSD Manual `ls(1)`:

| Tùy chọn | Mô tả chi tiết |
| :---: | :--- |
| `-A` | Liệt kê tất cả các mục trừ `.` và `..`. Tự động bật mặc định cho super-user (root). |
| `-a` | Liệt kê toàn bộ các mục bao gồm cả file ẩn bắt đầu bằng dấu chấm (`.`, `..`). |
| `-c` | Dùng thời gian thay đổi trạng thái file (`st_ctime`) khi sắp xếp (`-t`) hoặc in (`-l`). Ghi đè lẫn nhau với `-u`. |
| `-d` | Liệt kê chính thư mục như file thường (không duyệt đệ quy). Ghi đè lẫn nhau với `-R`. |
| `-F` | Thêm ký hiệu phân loại sau tên: `/` (thư mục), `*` (thực thi), `@` (symlink), `=` (socket), `\|` (FIFO). |
| `-f` | Tắt hoàn toàn việc sắp xếp và tự động bật `-a`. |
| `-h` | Hiển thị kích thước theo định dạng dễ đọc (B, K, M, G, T) trong `-s` và `-l`. Ghi đè `-k`. |
| `-i` | Hiển thị số inode (`st_ino`) của từng file. |
| `-k` | Tính dung lượng trong `-s` theo đơn vị Kilobytes (1024 bytes). Ghi đè `-h`. |
| `-l` | Định dạng danh sách dài (mode, nlink, owner, group, size/rdev, timestamp, name, target link). Ghi đè lẫn nhau với `-n`. |
| `-n` | Tương tự `-l`, nhưng hiển thị UID và GID dưới dạng số nguyên. Ghi đè lẫn nhau với `-l`. |
| `-q` | Thay thế các ký tự không in được bằng dấu `?`. Bật mặc định khi xuất ra terminal. Ghi đè lẫn nhau với `-w`. |
| `-R` | Duyệt và in đệ quy tất cả các thư mục con. Ghi đè lẫn nhau với `-d`. |
| `-r` | Đảo ngược thứ tự sắp xếp. |
| `-S` | Sắp xếp theo dung lượng file giảm dần (lớn nhất lên trước). |
| `-s` | Hiển thị số khối block thực tế được cấp phát cho file (hỗ trợ `BLOCKSIZE`, `-k`, `-h`). In `total` khi xuất ra terminal. |
| `-t` | Sắp xếp theo thời gian mới nhất lên trước. |
| `-u` | Dùng thời gian truy cập gần nhất (`st_atime`) khi sắp xếp (`-t`) hoặc in (`-l`). Ghi đè lẫn nhau với `-c`. |
| `-w` | In nguyên bản các ký tự không in được. Bật mặc định khi xuất ra pipe/file. Ghi đè lẫn nhau với `-q`. |

---

## 3. Cấu trúc mã nguồn Modular

Dự án được tổ chức thành các mô-đun độc lập với header và file thực thi tương ứng:

```text
NguyenThanhDo_24IT24IT335_midterm/
├── Makefile                # BSD/GNU Make tương thích, cờ -O2 -D_NETBSD_SOURCE
├── README.md               # Báo cáo kỹ thuật chi tiết
├── .gitignore              # Loại trừ build/, *.o, myls, sandbox
├── include/
│   ├── options.h           # Định nghĩa cấu trúc Options, enum TimeType, SizeFormat
│   ├── entry.h             # Cấu trúc Entry và EntryList (mảng động)
│   ├── sort.h              # Giao diện sắp xếp đa tiêu chí
│   ├── display.h           # Định dạng in ấn (long, short, blocks, inode, classify)
│   └── list.h              # Duyệt thư mục và đệ quy an toàn
├── src/
    ├── options.c           # Phân tích cờ dòng lệnh bằng getopt() & quy tắc override
    ├── entry.c             # Nạp metadata stat/lstat, cấp phát động và giải phóng
    ├── sort.c              # Sắp xếp theo tên, thời gian, dung lượng, đảo ngược, nosort
    ├── display.c           # Căn chỉnh cột bảng, format quyền, thời gian 6-tháng
    ├── list.c              # Đọc opendir/readdir, lọc -A/-a, ngăn vòng lặp symlink
    └── main.c              # Phân loại operand (file trước, dir sau), quản lý exit status
└── tests/
    └── test.sh             # Kịch bản kiểm thử tự động 16 test case chuẩn NetBSD
```

---

## 4. Xử lý trường hợp biên & Đảm bảo an toàn hệ thống

- **Chống lặp vô tận khi đệ quy (`-R`):** Sử dụng `lstat` khi đọc thư mục con, không dereference symbolic link để ngăn chặn triệt để vòng lặp liên kết (symlink cycles).
- **Phân loại operand:** File thường được gom và in trước, sau đó từng thư mục được in kèm tiêu đề `path:` và cách nhau bởi dòng trống.
- **Xử lý Dangling Symlink:** Không bị crash hay lỗi khi gặp symlink bị hỏng target.
- **Quản lý bộ nhớ:** Kiểm tra `NULL` cho mọi hàm cấp phát (`malloc`, `strdup`, `realloc`) và giải phóng hoàn toàn qua `entry_list_free()`, đảm bảo **0 memory leak**.
- **Mã thoát (Exit status):** Thoát `0` khi thành công và `>0` khi gặp lỗi truy cập hoặc sai cờ lệnh.

---

## 5. Hướng dẫn biên dịch và chạy trên NetBSD 10.1

### 5.1. Yêu cầu môi trường
Chương trình được xác nhận hoạt động trên **NetBSD 10.1**, sử dụng trình biên dịch `cc` theo chuẩn C11 và BSD Make. Để tải mã nguồn trực tiếp từ GitHub, máy ảo cần có Git và kết nối Internet.

- Kiểm tra các công cụ cần thiết:
```bash
cc --version
make --version
git --version
```
- Nếu chưa có Git, có thể cài đặt bằng:
```bash
pkgin -y install git
```

### 5.2. Tải mã nguồn từ GitHub
- Đăng nhập vào NetBSD, chuyển đến thư mục cá nhân của tài khoản hiện tại:
```bash
cd
```
- Clone dự án từ GitHub:
```bash
git clone https://github.com/Renber-vku/NguyenThanhDo_24IT24IT335_midterm.git
```
- Di chuyển vào thư mục dự án:
```bash
cd NguyenThanhDo_24IT24IT335_midterm
```
- Kiểm tra các tệp đã tải:
```bash
ls -la
```

### 5.3. Biên dịch
Chạy lệnh sau để biên dịch toàn bộ mô-đun và tạo tệp thực thi `myls`:
```bash
make
```
*Makefile sử dụng các cờ chính: `-std=c11`, `-Wall`, `-Wextra`, `-Wpedantic`, `-O2`, `-D_NETBSD_SOURCE` và `-D_POSIX_C_SOURCE=200809L`.*

### 5.4. Chạy chương trình
| Lệnh | Ý nghĩa |
| :--- | :--- |
| `./myls` | Liệt kê thư mục hiện tại. |
| `./myls -la` | Hiện tất cả mục với thông tin chi tiết. |
| `./myls -lh /etc` | Liệt kê `/etc` theo dạng dài và kích thước dễ đọc. |
| `./myls -R tests` | Duyệt đệ quy thư mục `tests`. |
| `./myls -S /etc` | Sắp xếp nội dung `/etc` theo kích thước giảm dần. |
| `./myls -F` | Thêm ký hiệu để phân biệt thư mục, tệp thực thi và liên kết. |

### 5.5. Dọn sản phẩm biên dịch
Xóa tệp `myls` và toàn bộ tệp đối tượng `.o` bằng lệnh:
```bash
make clean
```

---

## 6. Kiểm thử tự động (Automated Testing)

Bộ kiểm thử được chạy tự động bằng lệnh:
```bash
make test
```
*(Hoặc chạy trực tiếp `sh tests/test.sh`)*

Kịch bản `tests/test.sh` tạo dữ liệu tạm, chạy chương trình với nhiều tổ hợp tùy chọn và đối chiếu kết quả với `ls` của hệ thống. Trong lần chạy trên NetBSD 10.1, **16 trường hợp kiểm thử đều PASS** và kết thúc bằng thông báo `All tests passed (16/16)`:
- Ẩn tệp bắt đầu bằng dấu chấm ở chế độ mặc định; kiểm tra `-A` và `-a`.
- Phân loại thư mục, tệp thực thi và liên kết tượng trưng bằng `-F`.
- Định dạng dài, metadata và đích liên kết tượng trưng bằng `-l`.
- Duyệt thư mục con bằng `-R` và hành vi liệt kê chính thư mục bằng `-d`.
- Quy tắc ghi đè giữa `-l` và `-n`; sắp xếp bằng `-S` và `-r`.
- Hiển thị inode, block và kích thước dễ đọc bằng tổ hợp `-i`, `-s`, `-h`.
- Thay ký tự không in được bằng `-q` và trả lỗi khi toán hạng không tồn tại.

---

## 7. Hướng dẫn các lệnh Git

```bash
git init
git add .gitignore Makefile README.md include/ src/ tests/
git commit -m "feat: complete NetBSD 10.1 compliant ls(1) midterm project"
git branch -M main
git remote add origin https://github.com/Renber-vku/NguyenThanhDo_24IT24IT335_midterm.git
git push -u origin main
```
