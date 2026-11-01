// Package secretlib 代表公司内部的私有依赖，只存在于私有制品库。
package secretlib

// Message 返回一行文字，构建成功后程序会把它打印出来。
func Message() string {
	return "private module loaded from the artifact server"
}
