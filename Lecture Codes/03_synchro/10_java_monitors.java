// https://docs.oracle.com/javase/8/docs/api/java/lang/Object.html

public class bank_account {

	// protected shared resource
    double accounts[100];

    // a non-synchronized method
    public double current_balance(int i) {
        return accounts[i];
    }

	// compute the total balance across all accounts
	public synchronized double audit() {
		double balance = 0.0;
		for (int i = 0; i < 100; i++) {
			balance += accounts[i];
		}
		return balance;
	}

    // transfer "amount" from accounts[source] to accounts[target]
    public synchronized boolean transfer(double amount, int source, int target) {
        while (accounts[source] < amount) {
            wait();
        }
        accounts[source] -= amount;
        accounts[target] += amount;
        notifyAll();
        return true;
    }

}
