# libreoffice-docskill

DocSkill Office 的 LibreOffice **源码树**（可浏览的 `.cxx` / `.hxx` 等），不是 patch 备份。

- 默认分支：`docskill`（扁平快照，便于 GitHub 托管）
- 上游基线：`bd90af59c0078c2a015d526669bd9a896212736c`（LibreOffice core）
- **不含** `workdir/`、`instdir/`（本机编译产物）

## 与主仓关系

主产品仓：[soon365/docskill.ai](https://github.com/soon365/docskill.ai)  
专题可读补丁仍在主仓 `patches/`；**以本仓源码为准**做换机与协作。

## 本地开发

本机若已有带 `workdir` 的完整树，继续在 `C:\docskill.ai\LibreOffice` 开发，并把 `github` remote 指到本仓即可：

```powershell
cd C:\docskill.ai\LibreOffice
git remote add github https://github.com/soon365/libreoffice-docskill.git   # 若尚未添加
git fetch github
# 提交后：
git push github HEAD:docskill
```

全新 clone（无编译缓存，体积大）：

```powershell
git clone -b docskill https://github.com/soon365/libreoffice-docskill.git LibreOffice
```

然后按 DocSkill 构建文档执行 `autogen` / `make`。
