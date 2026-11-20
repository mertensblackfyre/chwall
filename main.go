package main

import (
	"context"
	"fmt"
	"io/fs"
	"log"
	"math/rand/v2"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"github.com/pelletier/go-toml/v2"
)

/*

 - Change hyprpaper then reload
 - Change the config using sed
 - Change hyprlock config
 - run matugen
 - reload quickshell

*/

type Config struct {
	FilePath string `toml:"filepath"`
}

func load_config(path string) (string, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return "", err
	}
	cfg := &Config{}
	if err := toml.Unmarshal(data, cfg); err != nil {
		return "", err
	}
	return cfg.FilePath, nil
}

func home_path(path string) string {
	home, err := os.UserHomeDir()
	if err != nil {
		return path
	}
	if path[:2] == "~/" {
		return filepath.Join(home, path[2:])
	}
	return path
}

func sed(file, pattern, replacement string) error {
	escaped := strings.NewReplacer(`\`, `\\`, `&`, `\&`, `|`, `\|`).Replace(replacement)
	expr := fmt.Sprintf("s|%s|\\1%s|", pattern, escaped)

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, "sed", "-i", "-E", expr, file)
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func random_wall() string {

	var chosen string
	var seen int

	cfg_path, err := load_config("config.toml")
	if err != nil {
		fmt.Println(err)
	}
	file_path := home_path(cfg_path)
	err = filepath.Walk(file_path, func(path string, info fs.FileInfo, err error) error {
		if err != nil {
			fmt.Printf("prevent panic by handling failure accessing a path %q: %v\n", path, err)
			return err
		}
		if !info.IsDir() {
			ext := strings.ToLower(filepath.Ext(path))
			if ext == ".png" || ext == ".jpg" || ext == ".jpeg" {
				seen++
				if rand.IntN(seen) == 0 {
					chosen = path
				}
			}
		}

		return nil
	})

	if err != nil {
		return ""
	}
	return chosen
}

func main() {
	wal := random_wall()

	if err := sed("hy.conf", `(path[[:space:]]*=.*)`, wal); err != nil {
		log.Println("sed error:", err)
	}
}
