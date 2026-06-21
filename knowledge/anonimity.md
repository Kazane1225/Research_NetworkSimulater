---

# Knowledge Base: History Trees and Their Applications in Anonymous Dynamic Networks

This document serves as a comprehensive core knowledge base combining the foundational architectural notes and advanced computational proofs regarding **History Trees** in anonymous dynamic networks.

---

## 1. Core Computational Model & Definitions

### 1.1 Network Architecture

* 
**System Representation**: A dynamic network is defined as an infinite sequence of undirected multigraphs $\mathcal{G} = (G_t)_{t \ge 1}$, where $G_t = (V, E_t)$.


* 
**Agents ($V$)**: A finite, non-empty set of $n$ independent computational units (processes) that lack unique identifiers (anonymous) and run the same local deterministic algorithm.


* 
**Links ($E_t$)**: Unordered pairs of elements in $V$ representing communication links that appear or disappear unpredictably at discrete synchronous time units called rounds. Parallel links and self-loops are explicitly permitted.


* 
**Communication Rules**: Agents can only interact by broadcasting messages to all immediate neighbors in $G_t$ through incident links. An agent cannot specify single destinations due to the network's anonymity.



### 1.2 Connectivity Parameters

* 
**Dynamic Disconnectivity ($\tau$)**: The network is defined as $\tau$-union-connected if the union of all edge sets across any $\tau$ consecutive rounds induces a connected graph on $V$. Mathematically, for all $i \ge 1$, the multigraph $(V, \bigcup_{t=i}^{i+\tau-1}E_t)$ is connected.


* 
**Dynamic Diameter ($d$)**: The maximum number of rounds required for information to travel from any single agent to any other agent in the network at any point in time.


* 
**Parameter Boundary**: In any $1$-union-connected network, $\tau \le d \le \tau(n - 1)$. If $\tau = 1$, the model simplifies to the classic $1$-interval-connected network model.



### 1.3 Target Functions for Computation

Computable tasks are mapped to specific classes of functions that map input $n$-tuples to output $n$-tuples:

$$\mu_{\lambda} = \{(z_1, m_1), (z_2, m_2), \dots, (z_k, m_k)\}$$

Where $z_i$ represents an input value and $m_i$ represents its exact multiplicity in the network ($n = \sum m_i$).

* 
**Multiset-Based Functions**: Functions where the output of an agent depends exclusively on its own input and the complete multiset of all agents' inputs: $F(p_i, \lambda) = \psi(\lambda(p_i), \mu_{\lambda})$.


* 
*Complete Problem*: **Input Multiset Function ($F_{IM}$)**, which outputs $\mu_{\lambda}$. Solving $F_{IM}$ solves all multiset-based tasks (e.g., **Counting** the exact network size $n$).




* 
**Frequency-Based Functions**: Functions that depend only on the relative frequency of each input value rather than their absolute multiplicities. For any positive integer $\alpha$, $\psi(z, \mu_{\lambda}) = \psi(z, \alpha \cdot \mu_{\lambda})$.


* 
*Complete Problem*: **Input Frequency Function ($F_{IF}$)**, which outputs $\frac{1}{n} \cdot \mu_{\lambda}$. Solving $F_{IF}$ solves all frequency-based tasks (e.g., **Average Consensus**, calculating statistical mean, variance, or median).





---

## 2. Theoretical Framework of History Trees

### 2.1 Inductive Indistinguishability

Symmetry dictates that agents can only be distinguished via their unique inputs or by receiving different multisets of messages.

1. 
**At Round $t = 0$**: Two agents are indistinguishable if and only if they have the same initial input.


2. 
**At Round $t > 0$**: Two agents $p$ and $q$ are indistinguishable if and only if they were indistinguishable at round $t-1$ and, for every equivalence class $A$ of indistinguishable agents at $t-1$, both $p$ and $q$ receive an identical number of messages from agents in $A$ during round $t$.



### 2.2 Structural Anatomy of a History Tree ($\mathcal{H}_{\mathcal{G}}$)

A history tree is an infinite graph subdivided into levels:

* 
**Nodes in Level $L_t$**: Represent the equivalence classes of agents that are indistinguishable at the end of round $t$. Level $L_{-1}$ contains a unique root node $r$ representing the entire system. Nodes in $L_0$ retain a label equal to their input value.


* 
**Anonymity $a(v)$**: The exact number of agents contained within the class represented by node $v$. Because nodes at any level $L_t$ partition the network, $\sum_{v \in L_t} a(v) = n$.


* 
**Black Edges**: Directed away from the root, inducing an infinite tree structure. A black edge $\{v, v'\}$ for $v \in L_{t-1}, v' \in L_t$ indicates that $v'$ is a subclass (subset) of agents derived from $v$. Consequently, $a(v) = \sum_{v_i \in children(v)} a(v_i)$.


* 
**Red Edges**: Directed edges representing message transfers between consecutive levels. A red edge $(v, u)$ with multiplicity $m$ ($v \in L_{t-1}, u \in L_t$) indicates that at round $t$, every individual agent in class $u$ received exactly $m$ identical messages from agents belonging to class $v$.



### 2.3 Local Agent Vistas

* 
**Definition**: A **vista** (historically termed "view" ) of an agent $p$ at round $t$ is the finite subgraph of $\mathcal{H}_{\mathcal{G}}$ induced by all nodes spanned by monotonic paths connecting the root $r$ and the agent's current node $h(p, t)$.


* 
**Fundamental Theorem of History Trees**: If all agents execute the same local deterministic algorithm $\mathcal{A}$, the internal state of an agent $p$ at the end of round $t$ is uniquely determined by a function $\mathcal{F}_{\mathcal{A}}$ of the vista of $p$ at round $t$. This mapping is independent of $p$.


* 
**Corollary**: All agents represented by the exact same node in level $L_t$ exhibit the identical internal state and output at round $t$. Thus, agents execute a universal construction algorithm ($\mathcal{A}^*$) to maintain and broadcast their vistas as their complete internal state without any loss of informational capacity.


* 
**Vista Size Constraints**: A local vista at round $t$ is fully represented using $O(t \cdot n^2 \log M)$ bits, where $M$ is the maximum number of messages an agent can receive in a single round ($M < n$ for simple graphs).


* 
**Information Boundary (Lemma 3.3)**: In a $1$-union-connected dynamic network, every node at level $L_t$ is guaranteed to be contained within the local vista of every node at level $L_{t'}$ for all $t' \ge t + n - 1$.



---

## 3. Distributed Algorithmic Frameworks

### 3.1 The Equation System Subroutine (The Core Mechanism)

The foundational approach to mining a local vista $V$ involves finding consecutive levels where no node splits into multiple subclasses.

* 
**Non-Branching Node**: A node $v \in L_t$ that possesses exactly one child in the vista ($|children(v)| = 1$).


* 
**Strand**: A path $(w_1, w_2, \dots, w_k)$ consisting entirely of non-branching nodes where $w_i$ is the parent of $w_{i+1}$.


* 
**Exposed Nodes**: Two non-branching nodes $v_1, v_2 \in L_t$ with children $v_1', v_2' \in L_{t+1}$ are exposed with multiplicity $(m_1, m_2)$ if red edges $\{v_1', v_2\}$ and $\{v_2', v_1\}$ exist with multiplicities $m_1 \ge 1$ and $m_2 \ge 1$ respectively.


* 
**Conservation Rule (Lemma 4.1)**: Because communication links are inherently bidirectional, if nodes $v_1, v_2$ are exposed with multiplicity $(m_1, m_2)$ in the history tree, their exact anonymities conform to:



$$m_1 \cdot a(v_1) = m_2 \cdot a(v_2)$$

```
  [ Listing 1: Homogeneous System Generation ]
  Input: Local Vista V with levels L_-1, L_0, ..., L_h
  Output: (t, S) where S is a system of linear equations
  -----------------------------------------------------------------------
  1. Assign s := 0
  2. For t := 0 to h:
  3.    If L_t contains a node with no children: Return (-1, Empty)
  4.    If L_t contains a node with more than one child: Assign s := t + 1
  5.    Else:
  6.       Let k := |L_s| = |L_t|
  7.       Construct graph G on the k strands connecting L_s to L_t
  8.       Edges in G exist between strands that contain exposed node pairs
  9.       If G is connected:
  10.         Let G' be a spanning tree of G
  11.         Assign S := Empty
  12.         For each edge {Pi, Pj} in G':
  13.            Find exposed nodes v1 in Pi and v2 in Pj with multiplicity (m1, m2)
  14.            Add equation "m1 * xi = m2 * xj" to S
  15.         Return (t, S)
  16. Return (-1, Empty)

```

### 3.2 Leaderless Networks (Frequency-Based Computations)

A task is deterministically computable in a leaderless network if and only if it is a frequency-based function.

#### Stabilizing Input Frequency

* 
**Mechanics**: Every agent executes Listing 1 on its current vista. If a valid system $S$ of $k-1$ independent linear equations is found, the rank of the coefficient matrix is exactly $k-1$. Using Gaussian elimination, every variable $x_i$ (representing the anonymity of node $w_i \in L_t$) is solved as a positive rational multiple of $x_1$ ($x_i = \alpha_i x_1$).


* 
**Output Derivation**: For each node $v_i \in L_0$, let $\beta_i = \sum_{w_j \in descendants(v_i)} \alpha_j$ and $\beta = \sum \beta_i$. The agent outputs the set of pairs $\{(label(v_i), \beta_i / \beta)\}$.


* 
**Bound**: Stabilizes on the mathematically exact input frequency value in at most **$\tau(2n - 2)$ rounds**.



#### Terminating Input Frequency

* 
**Assumptions**: Requires prior knowledge of $\tau$ and an empirical upper bound $N$ on the network size ($N \ge n$).


* 
**Condition**: The agent runs the stabilizing routine. It sets its explicit termination flag and stops if $t \ge 0$ and the current round $t'$ satisfies:



$$t' \ge t + \tau(N - 1) + 1$$

* 
**Bound**: Terminates correctly in at most **$\tau(n + N - 2)$ rounds**. If the dynamic diameter $d$ is known instead of $N$, the termination bound optimizes to $\tau(n - 1) + d$ rounds.



### 3.3 Networks with Leaders (Multiset-Based Computations)

A task is deterministically computable in a network with leaders if and only if it is a multiset-based function. This model assumes all agents share a-priori knowledge of the exact number of initial leaders $l \ge 1$.

#### Stabilizing Input Multiset

* 
**Mechanics**: Follows the relative ratio mapping derived from Listing 1. Once the rational fractions $\beta_i$ are extracted for all nodes in $L_0$, the agent isolates the subset of nodes $\{v_{j_1}, \dots, v_{j_{l'}}\} \subseteq L_0$ whose labels indicate their leader flags are set.


* 
**Scaling**: Compute the leader weight sum $\beta' = \sum_{i=1}^{l'} \beta_{j_i}$. The true absolute anonymity of each node $v_i \in L_0$ is scaled via $\gamma_i = l \cdot \beta_i / \beta'$.


* 
**Bound**: Stabilizes on the absolute counts in at most **$\tau(2n - 2)$ rounds**.



---

## 4. The Terminating Multi-Leader Counting Algorithm

### 4.1 The Disambiguation Dilemma

When $l > 1$, leaders can become distinguishable from each other, causing a single leader node in $\mathcal{H}_{\mathcal{G}}$ to branch into multiple sub-leaders with unknown individual counts. To bypass this, the algorithm treats leader branches conditionally.

### 4.2 The Verification Subroutine: `ApproxCount`

`ApproxCount(V, s, x, l)` assumes that a chosen leader node $\vartheta \in L_s$ has a conditional anonymity of exactly $x$ ($1 \le x \le l$), which implies a specific scaling discrepancy $\delta = x / a(\vartheta)$. It deduces conditional anonymities $a'(v) = \delta \cdot a(v)$ level by level.

1. 
**Guesser Criteria**: A node $u$ is a guesser if the conditional anonymities of all its current children $u_1, \dots, u_k$ in $V$ are determined, and $a'(u) = \sum a'(u_i)$.


2. 
**Guess Equation**: If a red edge exists between a node $v$ and a guesser $u$ with multiplicity $m \ge 1$, the conditional guess $g(v)$ is computed as:



$$g(v) = \frac{\sum_{i=1}^k a'(u_i) \cdot m_i}{m}$$

Where $m_i$ is the multiplicity of the red edge connecting child $u_i$ to the parent of $v$.
3. **Well-Spread Enforcement**: When a guess is placed on $v$, both $v$ and all its sibling nodes become *locked*, preventing them from receiving overlapping guesses.
4. **Heavy Node Rule (Lemma 5.5)**: A node $v$ is designated as *heavy* if the total count of guessed nodes within its local subtree $w(v)$ satisfies $w(v) \ge \lfloor g(v) \rfloor$. If a node is heavy and contains zero heavy descendants, it is mathematically verified to have a correct guess ($g(v) = a'(v)$). It is then marked as *counted*.
5. **Isle Completion**: Counted nodes form a *counting cut* if they form a boundary slicing through all active branches of the vista. Complete boundaries allow the algorithm to accurately sum the inner conditional counts.

```
  [ Listing 2: Terminating Multi-Leader Counting ]
  Input: Local Vista V, Known Leader Count l
  Output: Exact System Size n or "Unknown"
  -----------------------------------------------------------------------
  1. Assign n* := -1, s := 0, c := 0
  2. Let b := number of active leader branches currently visible in V
  3. While c <= l - b:
  4.    Assign t* := -1
  5.    For x := l downto 1:
  6.       Assign (n', t) := ApproxCount(V, s, x, l)
  7.       Assign t* := max(t*, t)
  8.       If n' == "MissingNodes": Return "Unknown"
  9.       If n' == "StrandTooShort": Break (Exit For Loop)
  10.      If n' != "WrongGuess":
  11.         If n* == -1: Assign n* := n'
  12.         Else if n* != n': Return "Unknown"
  13.         Assign c := c + 1
  14.         Break (Exit For Loop)
  15.   Assign s := t* + 1
  16. Let L_t' be the absolute final level currently present in V
  17. If t' >= t* + n*: Return n*
  18. Else: Return "Unknown"

```

### 4.3 Complexity & Convergence Boundaries

* 
**Universal Terminating Boundary**: Assuming $\tau$ and $l$ are known, Listing 2 terminates with the exact network size $n$ in at most:



$$\text{Rounds} \le \tau \cdot \left( (l^2 + l + 1)(n - 1) + 1 \right)$$

* 
**The Unique-Leader Case ($l = 1, \tau = 1$)**: The model simplifies drastically since the unique leader node can never branch.


* 
*Stabilization Lower Bound*: At least $2n - 6$ rounds are required. The paper's stabilizing algorithm settles in less than **$2n$ rounds** (worst-case optimal).


* 
*Termination Upper Bound*: The terminating algorithm completes execution in less than **$3n$ rounds**.





---

## 5. Completeness & Impossibility Theorems (The Guardrails)

The paper establishes strict mathematical limits showing that the architectural parameters used in the algorithms cannot be bypassed:

| Theorem Reference | Context Model | Constraint Condition | Proved Impossibility Result |
| --- | --- | --- | --- |
| <br>**Proposition 2.3** 

 | Leaderless Networks 

 | Global Unawareness of $\tau$ 

 | Explicitly terminating computation is impossible for **any** non-trivial function.

 |
| <br>**Proposition 6.1** 

 | Multi-Leader Networks 

 | System State Evaluation 

 | No deterministic algorithm can compute functions that are **not** multiset-based.

 |
| <br>**Proposition 6.2** 

 | Multi-Leader Networks 

 | Knowing ratio $n/l$ instead of $l$ 

 | Exact network size **Counting** becomes completely insolvable.

 |
| <br>**Proposition 6.3** 

 | Leaderless Networks 

 | System State Evaluation 

 | No deterministic algorithm can compute functions that are **not** frequency-based.

 |
| <br>**Proposition 6.4** 

 | Leaderless Networks 

 | Unawareness of upper bound $N$ or $d$ 

 | <br>**Average Consensus** cannot achieve terminating execution (finite termination fails).

 |

---

## 6. Asymptotically Optimal Lower Bounds

To confirm the tight efficiency of the history tree framework, the paper establishes lower bounds using families of simple graphs ($\mathcal{G}_m$) where symmetry breaking is maximally delayed:

### 6.1 Unique-Leader Configuration ($l = 1, \tau = 1$)

* 
**Stabilization**: No deterministic algorithm can compute the Counting function in less than **$2n - 6$ rounds**.


* 
**Termination**: No deterministic algorithm can terminate with the correct count in less than **$2n - 4$ rounds**.



### 6.2 Multi-Leader Configuration ($l > 1$)

* 
**Stabilization**: Computing the Counting function requires a minimum of **$\tau(2n - l - 5)$ rounds**.


* 
**Termination**: Terminating with the correct count requires a minimum of **$\tau(2n - l - 3)$ rounds**.



### 6.3 Leaderless Average Consensus

* 
**Stabilization**: Reaching consensus on the relative frequencies requires at least **$\tau(2n - 6)$ rounds**.


* 
**Termination**: Halting with a verified termination certificate requires at least **$\tau(2n - 4)$ rounds**.



---