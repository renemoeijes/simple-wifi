cat ~/.ssh/id_ed25519.pub
git config --global user.email "simpelmuis@gmail.com"
git config --global user.name "Rene Moeijes"
git init
git add .
git commit -m "Initial commit"
git branch -M main
git remote add origin git@github.com:renemoeijes/wifi-config-ap.git
git push --force -u origin main
